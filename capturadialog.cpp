#include "capturadialog.h"
#include "ui_capturadialog.h"
#include "database.h"
#include "camerathread.h"
CameraThread *camThread = nullptr;
#include <QDir>
#include <QMessageBox>
#include <QPixmap>
#include <QDebug>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonObject>
#include <QJsonDocument>

// Função utilitária: devolve o maior rosto detetado
static cv::Rect biggestFace(const std::vector<cv::Rect>& faces) {
    if (faces.empty()) return {};
    size_t idx = 0;
    int best = faces[0].area();
    for (size_t i = 1; i < faces.size(); ++i) {
        if (faces[i].area() > best) { best = faces[i].area(); idx = i; }
    }
    return faces[idx];
}


CapturaDialog::CapturaDialog(const QString &nome,
                             const QString &apelido,
                             const QString &dataNascimento,
                             cv::dnn::Net &net,
                             QWidget *parent)
    : QDialog(parent),
    ui(new Ui::CapturaDialog),
    nome(nome),
    apelido(apelido),
    dataNascimento(dataNascimento),
    mobileFaceNet(net)
{

    ui->setupUi(this);

    ui->labelStatus->setText("A iniciar...");

    // pasta do colaborador
    pastaColab = QString("rostos_colaboradores/%1_%2_%3")
                     .arg(this->nome, this->apelido, this->dataNascimento);
    QDir().mkpath(pastaColab);

    // cria registo na BD
    colaboradorId = Database::adicionarColaborador(this->nome,
                                                   this->apelido,
                                                   this->dataNascimento,
                                                   pastaColab);
    if (colaboradorId < 0) {
        ui->labelStatus->setText("Erro: BD não inicializada.");
    }
    http = new QNetworkAccessManager(this);

    // mantém o mesmo ID no Laravel (opcional mas recomendado para bater certo)
    enviarColaboradorParaLaravel();



    // carregar cascade
    if (!faceCascade.load("haarcascade_frontalface_default.xml")) {
        ui->labelStatus->setText("Erro: não encontrei haarcascade_frontalface_default.xml");
    }

    // abrir câmara

    capturando = true;
    ui->btnCapturar->setText("Capturar");
    ui->labelStatus->setText("Ajuste o rosto e clique Capturar.");
}
void CapturaDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);  // deixa a janela aparecer

    // Espera 300ms → evita freeze!
    QTimer::singleShot(300, this, [=]() {

        ui->labelStatus->setText("A iniciar webcam...");

        camThread = new CameraThread(this);
        if (!camThread->startCamera(0)) {
            ui->labelStatus->setText("Erro: webcam não iniciou!");
            return;
        }

        connect(camThread, &CameraThread::frameReady, this, [&](const cv::Mat &frame){
            ui->labelStatus->hide();           // 🟢 assim que há imagem → remove o texto
            cv::Mat frameCopy = frame.clone();
            QImage qimg = matToQImage(frameCopy);

            ui->labelVideo->setPixmap(
                QPixmap::fromImage(qimg).scaled(
                    ui->labelVideo->size(),
                    Qt::KeepAspectRatio,
                    Qt::SmoothTransformation
                    )
                );
        });

        ui->labelStatus->setText("Webcam iniciada 🚀");
    });
}

CapturaDialog::~CapturaDialog()
{
    if (timer) timer->stop();
    if (cap.isOpened()) cap.release();
    delete ui;
}
void CapturaDialog::on_btnCapturar_clicked()
{
    if (!capturando || !camThread) return;

    cv::Mat frame = camThread->getLastFrame();   // 👈 NOVO!

    if (frame.empty()) {
        QMessageBox::warning(this, "Erro", "Não foi possível obter frame da câmera.");
        return;
    }

    cv::Mat gray;
    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
    cv::equalizeHist(gray, gray);

    std::vector<cv::Rect> faces_cv;
    faceCascade.detectMultiScale(gray, faces_cv, 1.2, 5, 0, cv::Size(80,80));

    if (faces_cv.empty()) {
        QMessageBox::warning(this, "Aviso", "Nenhum rosto detetado!");
        return;
    }

    cv::Rect faceRect = biggestFace(faces_cv);
    cv::Mat face = frame(faceRect).clone();

    cv::Mat embedding = getFaceEmbedding(face);
    if (embedding.empty()) {
        QMessageBox::warning(this, "Erro", "Falha ao extrair embedding!");
        return;
    }

    QByteArray encodingBlob;
    encodingBlob.resize(embedding.total() * embedding.elemSize());
    memcpy(encodingBlob.data(), embedding.data, encodingBlob.size());

    contadorFotos++;
    if (colaboradorId > 0) {
        Database::adicionarFoto(colaboradorId, QString(), encodingBlob);
    }

    ui->labelStatus->setText(QString("Captura %1/5 guardada.").arg(contadorFotos));

    if (contadorFotos >= 5) {
        QMessageBox::information(this, "Concluído",
                                 "5 encodes guardados no banco de dados.");
        accept();
    }
}

void CapturaDialog::atualizarFrame()
{


    cv::Mat frame;
    cap >> frame;
    if (frame.empty()) return;

    // bounding box
    if (!faceCascade.empty()) {
        cv::Mat gray;
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
        cv::equalizeHist(gray, gray);
        std::vector<cv::Rect> faces;
        faceCascade.detectMultiScale(gray, faces, 1.2, 5, 0, cv::Size(80,80));
        if (!faces.empty()) {
            cv::rectangle(frame, biggestFace(faces), cv::Scalar(0,255,0), 2);
        }
    }

    QImage qimg = matToQImage(frame);
    ui->labelVideo->setPixmap(QPixmap::fromImage(qimg).scaled(
        ui->labelVideo->size(),
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation));
}
void CapturaDialog::enviarColaboradorParaLaravel()
{
    // Se não quiseres forçar o mesmo ID no Laravel, remove a linha do json["id"]
    QUrl url("http://127.0.0.1:8000/api/colaboradores");
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json["id"] = colaboradorId;            // 👈 mantém o mesmo ID no Laravel (opcional)
    json["nome"] = nome;
    json["apelido"] = apelido;
    json["data_nascimento"] = dataNascimento; // "YYYY-MM-DD"

    QNetworkReply *rp = http->post(req, QJsonDocument(json).toJson());
    connect(rp, &QNetworkReply::finished, this, [this, rp]() {
        const int status = rp->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray body = rp->readAll();

        if (rp->error() == QNetworkReply::NoError) {
            // Se não enviares "id", atualiza com o ID devolvido pelo Laravel:
            // auto obj = QJsonDocument::fromJson(body).object();
            // colaboradorId = obj["colaborador"].toObject()["id"].toInt(colaboradorId);

            qDebug() << "✅ Colaborador sincronizado com Laravel. HTTP" << status << "Body:" << body;
        } else {
            qWarning() << "❌ Erro ao sincronizar colaborador:" << rp->errorString()
                << "HTTP" << status << "Body:" << body;
        }
        rp->deleteLater();
    });
}

// ----------------------
// Funções auxiliares
// ----------------------

cv::Mat CapturaDialog::preprocessFace(const cv::Mat &face)
{
    cv::Mat resized, rgb, floatImg, blob;
    cv::resize(face, resized, cv::Size(112, 112));
    cv::cvtColor(resized, rgb, cv::COLOR_BGR2RGB);
    rgb.convertTo(floatImg, CV_32F, 1.0/127.5, -1.0); // normalização [-1,1]
    blob = cv::dnn::blobFromImage(floatImg);
    return blob;
}

cv::Mat CapturaDialog::getFaceEmbedding(const cv::Mat &face)
{
    cv::Mat inputBlob = preprocessFace(face);
    mobileFaceNet.setInput(inputBlob);
    cv::Mat embedding = mobileFaceNet.forward().clone(); // 1x128 ou 1x512
    return embedding.reshape(1, 1); // vetor único
}

QImage CapturaDialog::matToQImage(const cv::Mat &mat)
{
    if (mat.type() == CV_8UC3)
        return QImage(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_BGR888).copy();
    else if (mat.type() == CV_8UC1)
        return QImage(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_Grayscale8).copy();
    return QImage();
}
