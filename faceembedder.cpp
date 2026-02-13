#include "faceembedder.h"
#include <QDebug>
#include <QCoreApplication>   // 🔹 FALTAVA
#include <QDir>
FaceEmbedder::FaceEmbedder(QObject *parent)
    : QObject(parent)
{
    // ⚠️ caminho RELATIVO (sem hard-code absoluto)
    QString modelPath = QCoreApplication::applicationDirPath();
    QDir dir(modelPath);
    dir.cdUp();
dir.cdUp();     // sai de build/
    dir.cd("models");         // entra em models/
    modelPath = dir.filePath("MobileFaceNet.onnx");

    try {
        net = cv::dnn::readNetFromONNX(modelPath.toStdString());
        initialized = true;
        qDebug() << "MobileFaceNet carregado com sucesso";
    }
    catch (const cv::Exception &e) {
        qDebug() << "Erro ao carregar MobileFaceNet:" << e.what();
    }
}

void FaceEmbedder::extractEmbedding(const cv::Mat &faceRaw)
{

    if (!embedTimer.isValid())
        embedTimer.start();

    if (embedTimer.elapsed() < 120)   // ~8 FPS
        return;

    embedTimer.restart();

    if (!initialized || faceRaw.empty()) {
        qDebug() << "❌ Face vazia ou modelo não inicializado";
        return;
    }



    // ==============================
    // 1️⃣ Resize obrigatório
    // ==============================
    cv::Mat face;
    cv::resize(faceRaw, face, cv::Size(112,112));

    // ==============================
    // 2️⃣ Normalização MobileFaceNet
    // ==============================
    face.convertTo(face, CV_32F, 1.0/255.0);
    face = (face - 0.5f) / 0.5f;

    // ==============================
    // 3️⃣ Criar blob DNN
    // ==============================
    cv::Mat blob = cv::dnn::blobFromImage(
        face,
        1.0,
        cv::Size(112,112),
        cv::Scalar(0,0,0),
        false,
        false
        );

    net.setInput(blob);

    cv::Mat output = net.forward(); // 1x128



    std::vector<float> embedding(
        output.ptr<float>(),
        output.ptr<float>() + output.total()
        );

    float norm = 0.f;
    for (float v : embedding) norm += v*v;
    norm = std::sqrt(norm);
    for (float &v : embedding) v /= norm;
    emit embeddingReady(embedding);
}

