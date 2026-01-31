#include "capturastep.h"
#include "ui_mainwindow.h"
#include "database.h"
#include "camerathread.h"

#include <QDir>
#include <QMessageBox>
#include <QPixmap>
#include <QDebug>
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

CapturaStep::CapturaStep(Ui::MainWindow *ui,
                         CameraThread *camera,
                         cv::dnn::Net &net,
                         QObject *parent)
    : QObject(parent),
    ui(ui),
    camera(camera),
    mobileFaceNet(net)
{
    http = new QNetworkAccessManager(this);

    if (!faceCascade.load("haarcascade_frontalface_default.xml")) {
        qWarning() << "Erro: não encontrei haarcascade_frontalface_default.xml";
    }

    connect(ui->btnCapturar, &QPushButton::clicked,
            this, &CapturaStep::onBtnCapturarClicked);
}

void CapturaStep::startStep(const QString &nome,
                            const QString &apelido,
                            const QString &dataNascimento)
{
    this->nome = nome;
    this->apelido = apelido;
    this->dataNascimento = dataNascimento;

    contadorFotos = 0;
    capturando = true;

    ui->labelStatus->setText("A iniciar captura facial...");

    pastaColab = QString("rostos_colaboradores/%1_%2_%3")
                     .arg(nome, apelido, dataNascimento);
    QDir().mkpath(pastaColab);

    colaboradorId = Database::adicionarColaborador(nome, apelido, dataNascimento, pastaColab);
    enviarColaboradorParaLaravel();

    connect(camera, &CameraThread::frameReady,
            this, &CapturaStep::onFrameReceived);

    camera->startCamera();
}

void CapturaStep::stopStep()
{
    disconnect(camera, &CameraThread::frameReady,
               this, &CapturaStep::onFrameReceived);

    capturando = false;
}

void CapturaStep::onFrameReceived(const cv::Mat &frame)
{
    QImage qimg = matToQImage(frame);

    ui->labelVideo->setPixmap(
        QPixmap::fromImage(qimg).scaled(
            ui->labelVideo->size(),
            Qt::KeepAspectRatioByExpanding,
            Qt::SmoothTransformation
            )
        );
}

void CapturaStep::onBtnCapturarClicked()
{
    if (!capturando) return;

    cv::Mat frame = camera->getLastFrame();
    if (frame.empty()) return;

    cv::Mat gray;
    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
    cv::equalizeHist(gray, gray);

    std::vector<cv::Rect> faces;
    faceCascade.detectMultiScale(gray, faces, 1.2, 5, 0, cv::Size(80,80));

    if (faces.empty()) {
        ui->labelStatus->setText("Nenhum rosto detetado.");
        return;
    }

    cv::Rect faceRect = biggestFace(faces);
    cv::Mat face = frame(faceRect).clone();

    cv::Mat embedding = getFaceEmbedding(face);
    if (embedding.empty()) return;

    QByteArray encodingBlob;
    encodingBlob.resize(embedding.total() * sizeof(float));
    memcpy(encodingBlob.data(), embedding.data, encodingBlob.size());

    Database::adicionarFoto(colaboradorId, QString(), encodingBlob);

    contadorFotos++;
    ui->labelStatus->setText(QString("Captura %1/5").arg(contadorFotos));

    if (contadorFotos >= 5) {
        ui->labelStatus->setText("Captura concluída.");
        emit /* sinal para mudar para step de validação */;
    }
}

// ---------- Funções auxiliares (iguais às tuas) ----------

cv::Mat CapturaStep::preprocessFace(const cv::Mat &face)
{
    cv::Mat resized, rgb, floatImg, blob;
    cv::resize(face, resized, cv::Size(112, 112));
    cv::cvtColor(resized, rgb, cv::COLOR_BGR2RGB);
    rgb.convertTo(floatImg, CV_32F, 1.0/127.5, -1.0);
    blob = cv::dnn::blobFromImage(floatImg);
    return blob;
}

cv::Mat CapturaStep::getFaceEmbedding(const cv::Mat &face)
{
    cv::Mat inputBlob = preprocessFace(face);
    mobileFaceNet.setInput(inputBlob);
    cv::Mat embedding = mobileFaceNet.forward().clone();
    return embedding.reshape(1, 1);
}

QImage CapturaStep::matToQImage(const cv::Mat &mat)
{
    if (mat.type() == CV_8UC3)
        return QImage(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_BGR888).copy();
    else if (mat.type() == CV_8UC1)
        return QImage(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_Grayscale8).copy();
    return QImage();
}

void CapturaStep::enviarColaboradorParaLaravel()
{
    QUrl url("http://127.0.0.1:8000/api/colaboradores");
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json["id"] = colaboradorId;
    json["nome"] = nome;
    json["apelido"] = apelido;
    json["data_nascimento"] = dataNascimento;

    QNetworkReply *rp = http->post(req, QJsonDocument(json).toJson());
    connect(rp, &QNetworkReply::finished, this, [this, rp]() {
        const int status = rp->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray body = rp->readAll();

        if (rp->error() == QNetworkReply::NoError) {
            qDebug() << "✅ Colaborador sincronizado com Laravel. HTTP" << status;
        } else {
            qWarning() << "❌ Erro ao sincronizar colaborador:" << rp->errorString()
                << "HTTP" << status;
        }
        rp->deleteLater();
    });
}
