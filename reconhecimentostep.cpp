#include "reconhecimentostep.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>
#include <QFile>
#include <QStandardPaths>
#include <QDir>
#include <QCoreApplication>
#include <QFileInfo>
#include "facelandmarks_dummy.h"
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

ReconhecimentoStep::ReconhecimentoStep(QLabel *videoLabel,
                                       QLabel *statusLabel,
                                       CircularProgress *progress,
                                       CameraThread *camera,
                                       cv::dnn::Net &net,
                                       QObject *parent)
    : QObject(parent),
    labelVideo(videoLabel),
    labelStatus(statusLabel),
    progress(progress),
    camera(camera),
    mobileFaceNet(net)
{
    if (!faceCascade.load("haarcascade_frontalface_default.xml")) {
        qWarning() << "Erro: não encontrei haarcascade_frontalface_default.xml";
    }

    soundRecognized.setSource(QUrl::fromLocalFile("sons/correct.wav"));
    soundRecognized.setVolume(0.8f);

    soundUnrecognized.setSource(QUrl::fromLocalFile("sons/wrong.wav"));
    soundUnrecognized.setVolume(0.8f);


    faceLandmark = std::make_unique<FaceLandmarksDummy>();
}
void ReconhecimentoStep::start()
{
    labelStatus->setText("Reconhecimento ativo...");
    jaReconhecido = false;
    scanning = false;

    connect(camera, &CameraThread::frameReady,
            this, &ReconhecimentoStep::onFrame);

}

void ReconhecimentoStep::stop()
{
    disconnect(camera, &CameraThread::frameReady,
               this, &ReconhecimentoStep::onFrame);
}

void ReconhecimentoStep::onFrame(const cv::Mat &frame)
{
    if (frame.empty()) return;

    cv::Mat gray;
    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

    std::vector<cv::Rect> faces;
    faceCascade.detectMultiScale(gray, faces, 1.1, 3, 0, cv::Size(120,120));

    cv::Mat draw = frame.clone();

    if (!faces.empty()) {
        cv::Rect faceRect = biggestFace(faces);

        // --- Quadrado centrado ---
        int cx = faceRect.x + faceRect.width  / 2;
        int cy = faceRect.y + faceRect.height / 2;
        int size = int(std::max(faceRect.width, faceRect.height) * 1.35f);

        cv::Rect square(cx - size/2, cy - size/2, size, size);
        square &= cv::Rect(0, 0, frame.cols, frame.rows);

        cv::Mat faceCrop = frame(square).clone();

        // --- Landmarks (NORMALIZADOS) ---
        auto landmarks = faceLandmark->detect(faceCrop);

        for (const auto &p : landmarks)
        {
            int px = square.x + static_cast<int>(p.x * square.width);
            int py = square.y + static_cast<int>(p.y * square.height);

            cv::circle(draw,
                       cv::Point(px, py),
                       2,
                       cv::Scalar(255,255,255),
                       -1);
        }

        // --- Lógica de scan ---
        if (!scanning && !jaReconhecido) {
            scanY = faceRect.y;
            scanDirection = 1;
            scanning = true;
            scanTimer.restart();
            meioCiclos = 0;
        }

        if (scanning && !jaReconhecido) {
            scanY += scanDirection * scanSpeed;

            if (scanY >= faceRect.y + faceRect.height) {
                scanDirection = -1;
                meioCiclos++;
            }
            else if (scanY <= faceRect.y) {
                scanDirection = 1;
                meioCiclos++;

                if (scanTimer.elapsed() > 2000) {
                    cv::Mat face = frame(faceRect).clone();
                    cv::Mat embedding = getFaceEmbedding(face);

                    int id = reconhecerColaborador(embedding);
                    if (id > 0) {
                        QString ultimo = Database::obterUltimaMarcacao(id);
                        QString tipo = (ultimo.isEmpty() || ultimo == "saida") ? "entrada" : "saida";

                        if (Database::registarMarcacao(id, tipo)) {
                            labelStatus->setText(
                                QString("Colaborador %1 registado como %2")
                                    .arg(id).arg(tipo));

                            (tipo == "entrada" ? soundRecognized : soundUnrecognized).play();
                            enviarRegisto(id, tipo);
                        }
                    } else {
                        labelStatus->setText("Rosto não reconhecido!");
                        soundUnrecognized.play();
                    }

                    jaReconhecido = true;
                    scanning = false;
                    progress->setValue(100);
                    meioCiclos = totalMeioCiclos;
                }
            }

            float percentF = float(scanY - faceRect.y) / float(faceRect.height);
            percentF = std::clamp(percentF, 0.0f, 1.0f);

            float progressoGlobal =
                (float(meioCiclos) + percentF) / float(totalMeioCiclos);

            progress->setValue(int(std::clamp(progressoGlobal, 0.0f, 1.0f) * 100));
        }
    }
    else {
        scanning = false;
        jaReconhecido = false;
        meioCiclos = 0;
        progress->setValue(0);
    }

    QImage qimg = matToQImage(draw);
    labelVideo->setPixmap(
        QPixmap::fromImage(qimg).scaled(
            labelVideo->size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation));
}



void ReconhecimentoStep::enviarRegisto(int colaboradorId, const QString &tipo)
{
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);

    QUrl url("http://127.0.0.1:8000/api/registos");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json["colaborador_id"] = colaboradorId;
    json["tipo"] = tipo;

    QNetworkReply *reply = manager->post(request, QJsonDocument(json).toJson());

    connect(reply, &QNetworkReply::finished, this, [reply]() {
        reply->deleteLater();
    });
}

// ---------- utilitários ----------

QImage ReconhecimentoStep::matToQImage(const cv::Mat &mat)
{
    if (mat.type() == CV_8UC3)
        return QImage(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_BGR888).copy();
    else if (mat.type() == CV_8UC1)
        return QImage(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_Grayscale8).copy();
    return QImage();
}

cv::Mat ReconhecimentoStep::preprocessFace(const cv::Mat &face)
{
    cv::Mat resized, rgb, floatImg, blob;
    cv::resize(face, resized, cv::Size(112, 112));
    cv::cvtColor(resized, rgb, cv::COLOR_BGR2RGB);
    rgb.convertTo(floatImg, CV_32F, 1.0/127.5, -1.0);
    blob = cv::dnn::blobFromImage(floatImg);
    return blob;
}

cv::Mat ReconhecimentoStep::getFaceEmbedding(const cv::Mat &face)
{
    cv::Mat inputBlob = preprocessFace(face);
    mobileFaceNet.setInput(inputBlob);
    cv::Mat embedding = mobileFaceNet.forward().clone();
    cv::normalize(embedding, embedding);
    return embedding.reshape(1, 1);
}

int ReconhecimentoStep::reconhecerColaborador(const cv::Mat &faceDescriptor)
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

    return (bestSim > 0.5f) ? bestId : -1;
}
