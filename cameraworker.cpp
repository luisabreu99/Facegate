#include "cameraworker.h"
#include <QDebug>

CameraWorker::CameraWorker(QObject *parent)
    : QObject(parent)
{
}

CameraWorker::~CameraWorker()
{
    stopCamera();
}

void CameraWorker::startCamera()
{
    if (running)
        return;

    // abrir webcam
    cap.open(0);
    if (!cap.isOpened()) {
        qDebug() << "❌ Não foi possível abrir a webcam";
        return;
    }

    // resolução fixa (muito importante para estabilidade)
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
    cap.set(cv::CAP_PROP_FPS, 30);

    running = true;

    // timer = captura contínua
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout,
            this, &CameraWorker::grabFrame);

    timer->start(33); // ~30 FPS

    qDebug() << "📷 Camera iniciada";
}

void CameraWorker::stopCamera()
{
    running = false;

    if (timer) {
        timer->stop();
        timer->deleteLater();
        timer = nullptr;
    }

    if (cap.isOpened())
        cap.release();

    qDebug() << "🛑 Camera parada";
}

void CameraWorker::grabFrame()
{
    if (!running)
        return;

    cv::Mat frame;
    cap >> frame;

    if (frame.empty())
        return;

    // converter BGR → RGB
    cv::cvtColor(frame, frame, cv::COLOR_BGR2RGB);

    // converter cv::Mat → QImage
    QImage img(frame.data,
               frame.cols,
               frame.rows,
               frame.step,
               QImage::Format_RGB888);

    emit frameReady(img.copy()); // copy MUITO IMPORTANTE
}
