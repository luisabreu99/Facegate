#include "reconhecimentodialog.h"
#include "ui_reconhecimentodialog.h"

#include <QMessageBox>
#include <QDebug>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonObject>
#include <QJsonDocument>
// Função utilitária: devolve o maior rosto
static cv::Rect biggestFace(const std::vector<cv::Rect>& faces) {
    if (faces.empty()) return {};
    size_t idx = 0;
    int best = faces[0].area();
    for (size_t i = 1; i < faces.size(); ++i) {
        if (faces[i].area() > best) { best = faces[i].area(); idx = i; }
    }
    return faces[idx];
}

ReconhecimentoDialog::ReconhecimentoDialog(QWidget *parent, cv::dnn::Net &net)
    : QDialog(parent),
    ui(new Ui::ReconhecimentoDialog),
    mobileFaceNet(net)
{
    ui->setupUi(this);
    ui->labelStatus->setText("A iniciar reconhecimento...");

    if (!faceCascade.load("haarcascade_frontalface_default.xml")) {
        ui->labelStatus->setText("Erro: não encontrei haarcascade_frontalface_default.xml");
    }

    // --- Sons ---
    soundRecognized.setSource(QUrl::fromLocalFile("sons/correct.wav"));
    soundRecognized.setVolume(0.8f);

    soundUnrecognized.setSource(QUrl::fromLocalFile("sons/wrong.wav"));
    soundUnrecognized.setVolume(0.8f);
}


void ReconhecimentoDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);

    if (!cap.isOpened()) {
        if (!cap.open(0)) {
            ui->labelStatus->setText("Erro ao abrir webcam.");
            return;
        }
        cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
        cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);

        timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &ReconhecimentoDialog::atualizarFrame);
        timer->start(30);

        ui->labelStatus->setText("Reconhecimento ativo.");
    }
}


ReconhecimentoDialog::~ReconhecimentoDialog()
{
    if (timer) timer->stop();
    if (cap.isOpened()) cap.release();
    delete ui;
}

void ReconhecimentoDialog::atualizarFrame()
{
    cv::Mat frame;
    cap >> frame;
    if (frame.empty()) return;

    cv::Mat gray;
    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
    std::vector<cv::Rect> faces;
    faceCascade.detectMultiScale(gray, faces, 1.2, 5, 0, cv::Size(80,80));

    if (!faces.empty()) {
        cv::Rect faceRect = biggestFace(faces);

        // Iniciar scan se não começou
        if (!scanning && !jaReconhecido) {
            scanY = faceRect.y;
            scanDirection = 1;
            scanning = true;
            scanSpeed = 40;
            scanTimer.restart();
        }

        if (scanning && !jaReconhecido) {
            // mover linha
            scanY += scanDirection * scanSpeed;

            if (scanY >= faceRect.y + faceRect.height) {
                scanDirection = -1;
            } else if (scanY <= faceRect.y) {
                scanDirection = 1;

                // reconhecimento só no topo
                if (scanTimer.elapsed() > 2000) { // 2s ciclo
                    cv::Mat face = frame(faceRect).clone();
                    cv::Mat embedding = getFaceEmbedding(face);

                    int id = reconhecerColaborador(embedding);
                    if (id > 0) {
                        QString ultimo = Database::obterUltimaMarcacao(id);
                        QString tipo;

                        if (ultimo.isEmpty() || ultimo == "saida") {
                            tipo = "entrada";
                        } else {
                            tipo = "saida";
                        }

                        if (Database::registarMarcacao(id, tipo)) {
                            ui->labelStatus->setText(
                                QString("Colaborador %1 registado como %2")
                                    .arg(id).arg(tipo)
                                );

                            if (tipo == "entrada")
                                soundRecognized.play();
                            else
                                soundUnrecognized.play();

                            enviarRegisto(id, tipo);

                        }
                    } else {
                        ui->labelStatus->setText("Rosto não reconhecido!");
                        soundUnrecognized.play();
                    }

                    jaReconhecido = true;   // ✅ só uma vez por rosto
                    scanning = false;       // parar scan até rosto sair
                }
            }

            // desenhar apenas a barra do scan
            cv::line(frame,
                     cv::Point(faceRect.x, scanY),
                     cv::Point(faceRect.x + faceRect.width, scanY),
                     cv::Scalar(0,255,0), 2);
        }
    } else {
        // Reset quando rosto desaparece
        scanning = false;
        jaReconhecido = false;
    }

    QImage qimg = matToQImage(frame);
    ui->labelVideo->setPixmap(QPixmap::fromImage(qimg).scaled(
        ui->labelVideo->size(),
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation));
}


void ReconhecimentoDialog::enviarRegisto(int colaboradorId, const QString &tipo)
{
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);

    QUrl url("http://127.0.0.1:8000/api/registos"); // 👉 Laravel API local
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // Criar JSON
    QJsonObject json;
    json["colaborador_id"] = colaboradorId;
    json["tipo"] = tipo;

    QJsonDocument doc(json);
    QByteArray data = doc.toJson();

    QNetworkReply *reply = manager->post(request, data);

    connect(reply, &QNetworkReply::finished, this, [reply]() {
        QByteArray body = reply->readAll();
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (reply->error() == QNetworkReply::NoError) {
            qDebug() << "✅ HTTP" << status << "Body:" << body;
        } else {
            qDebug() << "❌ HTTP" << status << reply->errorString() << "Body:" << body;
        }
        reply->deleteLater();
    });

}

// ----------------------
// Conversão OpenCV → Qt
// ----------------------
QImage ReconhecimentoDialog::matToQImage(const cv::Mat &mat)
{
    if (mat.type() == CV_8UC3)
        return QImage(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_BGR888).copy();
    else if (mat.type() == CV_8UC1)
        return QImage(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_Grayscale8).copy();
    return QImage();
}

// ----------------------
// MobileFaceNet utilitários
// ----------------------
cv::Mat ReconhecimentoDialog::preprocessFace(const cv::Mat &face)
{
    cv::Mat resized, rgb, floatImg, blob;
    cv::resize(face, resized, cv::Size(112, 112));
    cv::cvtColor(resized, rgb, cv::COLOR_BGR2RGB);
    rgb.convertTo(floatImg, CV_32F, 1.0/127.5, -1.0); // normalizar [-1,1]
    blob = cv::dnn::blobFromImage(floatImg);
    return blob;
}

cv::Mat ReconhecimentoDialog::getFaceEmbedding(const cv::Mat &face)
{
    cv::Mat inputBlob = preprocessFace(face);
    mobileFaceNet.setInput(inputBlob);
    cv::Mat embedding = mobileFaceNet.forward().clone(); // 1x128 ou 1x512
    cv::normalize(embedding, embedding); // normalização L2
    return embedding.reshape(1, 1);
}

// ----------------------
// Comparação com BD
// ----------------------
int ReconhecimentoDialog::reconhecerColaborador(const cv::Mat &faceDescriptor)
{
    QVector<QPair<int, QByteArray>> encodings = Database::obterEncodings();

    int bestId = -1;
    float bestSim = -1.0f;

    for (const auto &item : encodings) {
        const int id = item.first;
        const QByteArray &blob = item.second;

        if (blob.size() != faceDescriptor.total() * sizeof(float)) continue;

        const float *data = reinterpret_cast<const float*>(blob.constData());
        cv::Mat dbDescriptor(1, faceDescriptor.cols, CV_32F, (void*)data);

        float dot = faceDescriptor.dot(dbDescriptor);
        float sim = dot / (cv::norm(faceDescriptor) * cv::norm(dbDescriptor));

        if (sim > bestSim) {
            bestSim = sim;
            bestId = id;
        }
    }

    // Threshold MobileFaceNet: ~0.5
    return (bestSim > 0.5f) ? bestId : -1;
}
