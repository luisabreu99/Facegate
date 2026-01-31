#include "mainwindow.h"
#include "database.h"
#include <QApplication>
#include <QLocale>
#include <QTranslator>

#include <opencv2/dnn.hpp>
#include <opencv2/opencv.hpp>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "Reconhecimento_Facial_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }

    if (!Database::inicializar()) {
        return -1;  // não arrancar se BD falhar
    }

    // --- Warm-up MobileFaceNet ---
    cv::dnn::Net faceNet = cv::dnn::readNetFromONNX("models/MobileFaceNet.onnx");
    for (auto layerName : faceNet.getLayerNames()) {
        int id = faceNet.getLayerId(layerName);
        auto layer = faceNet.getLayer(id);

        if (layer->blobs.size() > 0) {
            cv::Mat blob = layer->blobs[0];
            qDebug() << "Layer:" << QString::fromStdString(layerName)
                     << " -> Tipo:" << blob.type() << "  (CV_32F = FP32, CV_16F = FP16)";
            break; // basta ver 1
        }
    }
    // cria um input falso só para compilar o grafo
    cv::Mat dummy = cv::dnn::blobFromImage(
        cv::Mat::zeros(112, 112, CV_8UC3),
        1.0/127.5, cv::Size(112, 112),
        cv::Scalar(127.5,127.5,127.5),
        true, false
        );
    cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_SILENT);
    faceNet.setInput(dummy);
    faceNet.forward(); // ⚡ primeiro forward "dummy" -> warm-up

    MainWindow w(nullptr, faceNet);
    w.show();



    return a.exec();
}
