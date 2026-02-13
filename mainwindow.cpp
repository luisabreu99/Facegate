#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>

#include <QPainter>
#include <QPen>
#include <QElapsedTimer>

#include <QPixmap>
#include <QPalette>
#include <QResizeEvent>


static QElapsedTimer recognitionTimer;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    qRegisterMetaType<QImage>("QImage");   // ⭐ FALTAVA ESTA LINHA
    qRegisterMetaType<cv::Mat>("cv::Mat");
    qRegisterMetaType<
        std::array<cv::Point3f,
                   CLFML::FaceMesh::NUM_OF_FACE_MESH_POINTS>
        >("FaceMeshPoints");
    ui->setupUi(this);


    // força primeira renderização das imagens



    QPixmap bg(":/images/background.png");
    bg = bg.scaled(this->size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    QPalette palette;
    palette.setBrush(QPalette::Window, bg);
    this->setPalette(palette);
    this->setAutoFillBackground(true);


    ui->stackedWidget->setCurrentWidget(ui->pageInicio);

    // 🔹 localizar colaboradores.db
    QDir dir(QCoreApplication::applicationDirPath());
    dir.cdUp();   // sai de FaceGateQt-Debug
    dir.cdUp();   // sai de build
    dir.cd("data");
    QString dbPath = dir.filePath("colaboradores.db");

    databaseManager.open(dbPath);
    attendanceService = new AttendanceService(&databaseManager);

    if (faceDatabase.load(dbPath.toStdString())) {
        databaseLoaded = true;
        qDebug() << "Base de dados carregada com sucesso";
    } else {
        qDebug() << "Erro ao carregar base de dados";
    }

    apiService = new ApiService(this);


}

MainWindow::~MainWindow()
{
    stopCameraPipeline();
   delete attendanceService;
    delete ui;
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    // 🔵 BACKGROUND
    QPixmap bg(":/images/background.png");
    bg = bg.scaled(this->size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    QPalette palette;
    palette.setBrush(QPalette::Window, bg);
    this->setPalette(palette);

    // 🟢 BOTÃO CAPTURAR


    QMainWindow::resizeEvent(event);
}


void MainWindow::startCameraPipeline()
{
   stopCameraPipeline();

    // ================= CAMERA =================
    cameraThread = new QThread(this);
    cameraWorker = new CameraWorker();
    cameraWorker->moveToThread(cameraThread);

    connect(cameraThread, &QThread::started,
            cameraWorker, &CameraWorker::startCamera);

    connect(cameraWorker, &CameraWorker::frameReady,
            this, &MainWindow::onFrameProcessed,
            Qt::QueuedConnection);

    cameraThread->start();

    // ================= FACE DETECTION =================
    detectionThread = new QThread(this);
    faceDetectionWorker = new FaceDetectionWorker();
    faceDetectionWorker->moveToThread(detectionThread);

    connect(this, &MainWindow::frameForDetection,
            faceDetectionWorker, &FaceDetectionWorker::setFrame,
            Qt::QueuedConnection);


    connect(faceDetectionWorker, &FaceDetectionWorker::faceReady,
            this, &MainWindow::faceForEmbedding,
            Qt::QueuedConnection);
    connect(faceDetectionWorker, &FaceDetectionWorker::faceRectReady,
            this, [this](const QRect &r){
                lastFaceRect = r;
            faceVisible = true;        // ⭐ FALTAVA ISTO
            faceLostTimer.restart();
            },
            Qt::QueuedConnection);

    detectionThread->start();
    QMetaObject::invokeMethod(faceDetectionWorker, "start");

    // ================= FACE EMBEDDER =================
    embedderThread = new QThread(this);
    faceEmbedder = new FaceEmbedder();
    faceEmbedder->moveToThread(embedderThread);

    connect(this, &MainWindow::faceForEmbedding,
            faceEmbedder, &FaceEmbedder::extractEmbedding,
            Qt::QueuedConnection);

    connect(faceEmbedder, &FaceEmbedder::embeddingReady,
            this, &MainWindow::onEmbeddingReady,
            Qt::QueuedConnection);

    embedderThread->start();
    // ================= FACE MESH (VISUAL ONLY) =================
    meshThread = new QThread(this);
    faceMeshWorker = new FaceMeshWorker();
    faceMeshWorker->moveToThread(meshThread);

    // ⭐ AGORA SIM — ligar frame → facemesh
    connect(faceDetectionWorker, &FaceDetectionWorker::faceReady,
            faceMeshWorker, &FaceMeshWorker::processFrame,
            Qt::QueuedConnection);


    // ⭐ landmarks → mainwindow
    connect(faceMeshWorker, &FaceMeshWorker::landmarksReady,
            this, &MainWindow::onLandmarksReady,
            Qt::QueuedConnection);

    meshThread->start();
}


void MainWindow::stopCameraPipeline()
{
    if (cameraWorker)
        cameraWorker->stopCamera();

    if (cameraThread) {
        cameraThread->quit();
        cameraThread->wait();
        cameraThread = nullptr;
        cameraWorker = nullptr;
    }



    if (embedderThread) {
        embedderThread->quit();
        embedderThread->wait();
        embedderThread = nullptr;
        faceEmbedder = nullptr;
    }


    if (detectionThread) {
        detectionThread->quit();
        detectionThread->wait();
        detectionThread = nullptr;
        faceDetectionWorker = nullptr;
    }

    if (meshThread) {
        meshThread->quit();
        meshThread->wait();
        meshThread = nullptr;
        faceMeshWorker = nullptr;
    }

}



void MainWindow::on_btnCapturar_clicked()
{
    appMode = AppMode::Recognition;
    startCameraPipeline();
    ui->stackedWidget->setCurrentWidget(ui->pageCaptura);
}

void MainWindow::onEmbeddingReady(const std::vector<float> &embedding)
{

    static QElapsedTimer timer;

    // 🔒 BLOQUEIO TOTAL FORA DO MODO CORRETO
    if (appMode == AppMode::Creating) {

        if (!extractingFace)
            return;

        if (collectedEmbeddings.size() >= REQUIRED_EMBEDDINGS)
            return;

        collectedEmbeddings.push_back(embedding);

        ui->lblContagem->setText(
            QString("Extração %1 / %2")
                .arg(collectedEmbeddings.size())
                .arg(REQUIRED_EMBEDDINGS)
            );

        if (collectedEmbeddings.size() == REQUIRED_EMBEDDINGS) {
            extractingFace = false;
            finalizeColaborador();
        }

        return; // ⛔ NUNCA entra no reconhecimento
    }

    // =============================
    // 🔍 MODO RECONHECIMENTO
    // =============================
    if (appMode != AppMode::Recognition)
        return;

    if (!timer.isValid())
        timer.start();

    if (timer.elapsed() < 2000)
        return;

    timer.restart();

    double score = 0.0;
    int colaboradorId =
        faceDatabase.findBestMatch(embedding, score);

    if (colaboradorId != -1 && score > 0.50) {
     qDebug() << "🔎 A comparar com base de dados...";
        QDateTime now = QDateTime::currentDateTime();

        // 🔒 ANTI-SPAM
        if (lastRegisterTime.count(colaboradorId)) {
            int secs = lastRegisterTime[colaboradorId].secsTo(now);
            if (secs < REGISTER_COOLDOWN) {
                qDebug() << "Cooldown ativo para ID" << colaboradorId;
                return;
            }
        }

        // 🔁 ENTRADA / SAÍDA
        QString lastType = attendanceService->getLastRegisterType(colaboradorId);
        QString newType = (lastType == "ENTRADA") ? "SAIDA" : "ENTRADA";

        attendanceService->insertRegister(colaboradorId, newType);
        apiService->sendRegisto(colaboradorId, newType);

        lastRegisterTime[colaboradorId] = now;

        qDebug() << "✔ Registo:" << newType
                 << "para colaborador" << colaboradorId;

        ui->statusbar->showMessage(
            QString("✔ %1 registada").arg(newType), 3000);
    } else {
        qDebug() << "❌ Desconhecido";
    }
    qDebug() << "Score:" << score << "ID:" << colaboradorId;
}

void MainWindow::onFrameProcessed(const QImage &img)
{
    lastFrame = img;
    emit frameForDetection(lastFrame);
    QImage frameToShow = lastFrame.copy();

    // 🧠 verificar se face desapareceu
    if (faceVisible && faceLostTimer.elapsed() > 700)
    {
        faceVisible = false;
        targetLandmarks.clear();
        drawLandmarks.clear();
    }

    // ⭐ INTERPOLAÇÃO SUAVE (o que faltava!!)
    if (!targetLandmarks.empty() && drawLandmarks.size() == targetLandmarks.size())
    {
        float smooth = 0.55f;   // quanto menor = mais suave

        for (size_t i = 0; i < drawLandmarks.size(); i++)
        {
            drawLandmarks[i].setX(
                drawLandmarks[i].x() + (targetLandmarks[i].x() - drawLandmarks[i].x()) * smooth
                );

            drawLandmarks[i].setY(
                drawLandmarks[i].y() + (targetLandmarks[i].y() - drawLandmarks[i].y()) * smooth
                );
        }
    }

    // 🎨 desenhar landmarks
    if (faceVisible && !drawLandmarks.empty() && !lastFaceRect.isNull())
    {
        QPainter painter(&frameToShow);
        painter.setBrush(Qt::white);
        painter.setPen(Qt::NoPen);

        for (const auto &p : drawLandmarks)
        {
            float fx = p.x() / 192.0f;
            float fy = p.y() / 192.0f;

            int x = lastFaceRect.x() + fx * lastFaceRect.width();
            int y = lastFaceRect.y() + fy * lastFaceRect.height();

            painter.drawEllipse(QPoint(x,y), 2, 2);
        }
    }

    QLabel *targetLabel = nullptr;

    if (appMode == AppMode::Recognition)
        targetLabel = ui->lblCamera;
    else if (appMode == AppMode::Creating)
        targetLabel = ui->lblExtract;

    if (targetLabel)
    {
        targetLabel->setPixmap(
            QPixmap::fromImage(frameToShow)
                .scaled(targetLabel->size(),
                        Qt::KeepAspectRatio,
                        Qt::FastTransformation));
    }

}

void MainWindow::on_btnCriarColab_clicked()
{
    appMode = AppMode::Idle;
    ui->stackedWidget->setCurrentWidget(ui->pageCriarColab);
    ui->stackedWidget_2->setCurrentWidget(ui->pageForm);

    collectedEmbeddings.clear();
    extractingFace = false;
}
void MainWindow::on_btnFormVoltar_clicked()
{


    ui->stackedWidget->setCurrentWidget(ui->pageInicio);
}

void MainWindow::on_btnVoltar_clicked()
{
    stopCameraPipeline();
    appMode = AppMode::Idle;
    ui->stackedWidget->setCurrentWidget(ui->pageInicio);
}
void MainWindow::on_btnFormContinuar_clicked()
{
    if (ui->leNome->text().isEmpty() ||
        ui->leApelido->text().isEmpty()) {
        qDebug() << "Campos obrigatórios";
        return;
    }

    appMode = AppMode::Creating;

    // ⭐ reset pipeline para modo captura
    QTimer::singleShot(0, this, [this](){
        stopCameraPipeline();
        startCameraPipeline();
    });

    collectedEmbeddings.clear();
    extractingFace = true;

    ui->lblContagem->setText("Extração 0 / 5");
    ui->stackedWidget_2->setCurrentWidget(ui->pageExtract);
}


void MainWindow::finalizeColaborador()
{
    qDebug() << "Extração concluída";

    // =================================
    // 1️⃣ MÉDIA DO EMBEDDING (continua aqui)
    // =================================
    std::vector<float> mean(128, 0.f);

    for (const auto &e : collectedEmbeddings)
        for (int i = 0; i < 128; ++i)
            mean[i] += e[i];

    for (float &v : mean)
        v /= collectedEmbeddings.size();

    // normalização L2
    float norm = 0.f;
    for (float v : mean) norm += v*v;
    norm = std::sqrt(norm);
    for (float &v : mean) v /= norm;

    // =================================
    // 2️⃣ AGORA A DB FICA NO SERVICE 💥
    // =================================
    int id = attendanceService->createColaboradorWithEmbedding(
        ui->leNome->text(),
        ui->leApelido->text(),
        ui->deDataNasc->date().toString("yyyy-MM-dd"),
        mean
        );


    if (id == -1)
    {
        qDebug() << "Erro ao guardar colaborador";
        return;
    }
    apiService->sendColaborador(
        ui->leNome->text(),
        ui->leApelido->text(),
        ui->deDataNasc->date().toString("yyyy-MM-dd"),
        mean
        );

    // recarregar embeddings na memória
    QDir dir(QCoreApplication::applicationDirPath());
    dir.cdUp(); dir.cdUp(); dir.cd("data");
    faceDatabase.load(dir.filePath("colaboradores.db").toStdString());

    qDebug() << "✔ Colaborador criado com sucesso";

    // =================================
    // 3️⃣ LIMPAR UI (fica no MainWindow)
    // =================================
    appMode = AppMode::Idle;
    extractingFace = false;
    collectedEmbeddings.clear();

    ui->stackedWidget->setCurrentWidget(ui->pageInicio);
    ui->stackedWidget_2->setCurrentWidget(ui->pageForm);
    stopCameraPipeline();

    ui->leNome->clear();
    ui->leApelido->clear();
    ui->deDataNasc->setDate(QDate::currentDate());
    ui->lblContagem->setText("Rosto registado com sucesso ✔");
}


void MainWindow::onFaceDetected(const QImage &faceROI)
{

    if (faceROI.isNull())
        return;
    faceVisible = true;
    faceLostTimer.restart();
    // =========================================
    // ⏱️ LIMITAR RECONHECIMENTO A 1 SEGUNDO
    // =========================================
    if (appMode == AppMode::Recognition)
    {
        if (!recognitionTimer.isValid())
            recognitionTimer.start();

        if (recognitionTimer.elapsed() < 1000)
            return; // ⛔ ainda não passou 1 segundo

        recognitionTimer.restart();
    }



    // =========================================
    // 🔄 QImage → cv::Mat
    // =========================================
    QImage rgb = faceROI.convertToFormat(QImage::Format_RGB888);

    cv::Mat mat(rgb.height(),
                rgb.width(),
                CV_8UC3,
                (void*)rgb.bits(),
                rgb.bytesPerLine());

    cv::Mat face;
    cv::cvtColor(mat, face, cv::COLOR_RGB2BGR);

    // =========================================
    // 📏 Resize obrigatório MobileFaceNet
    // =========================================
    cv::resize(face, face, cv::Size(112,112));

    // =========================================
    // 🔬 Normalização MobileFaceNet
    // =========================================
    face.convertTo(face, CV_32F, 1.0/255.0);
    face = (face - 0.5f) / 0.5f;

    // =========================================
    // 🚀 Enviar para embedder
    // =========================================
    emit faceForEmbedding(face);
}
void MainWindow::onLandmarksReady(
    const std::array<cv::Point3f,
                     CLFML::FaceMesh::NUM_OF_FACE_MESH_POINTS> &points)
{
    targetLandmarks.clear();

    for (const auto &p : points)
        targetLandmarks.emplace_back(p.x, p.y);

    // ⭐ primeira vez inicializar buffer de desenho
    if (drawLandmarks.empty())
        drawLandmarks = targetLandmarks;
}

