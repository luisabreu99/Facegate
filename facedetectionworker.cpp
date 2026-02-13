#include "facedetectionworker.h"
#include <QDebug>

FaceDetectionWorker::FaceDetectionWorker(QObject *parent)
    : QObject(parent)
{
    std::string path =
        "/usr/share/opencv4/haarcascades/haarcascade_frontalface_default.xml";

    if (!faceCascade.load(path))
        qDebug() << "❌ Erro ao carregar Haar Cascade";
    else
        qDebug() << "🙂 Haar Cascade carregado";

    // ⭐ loop interno a 30 FPS

}
void FaceDetectionWorker::start()
{
    timer = new QTimer(this);

    connect(timer, &QTimer::timeout,
            this, &FaceDetectionWorker::process);

    timer->start(33);

    qDebug() << "🧠 FaceDetection loop iniciado";
}


void FaceDetectionWorker::process()
{
    QImage frame;

    // obter último frame disponível
    {
        QMutexLocker locker(&mutex);
        if (latestFrame.isNull())
            return;
        frame = latestFrame.copy();
    }

    QImage rgb = frame.convertToFormat(QImage::Format_RGB888);

    cv::Mat mat(rgb.height(), rgb.width(),
                CV_8UC3,
                const_cast<uchar*>(rgb.bits()),
                rgb.bytesPerLine());

    cv::Mat gray;
    cv::cvtColor(mat, gray, cv::COLOR_RGB2GRAY);

    std::vector<cv::Rect> faces;
    faceCascade.detectMultiScale(gray, faces, 1.1, 6, 0, cv::Size(120,120));

    if (faces.empty())
        return;

    // maior face
    cv::Rect faceRect = faces[0];
    for (auto &r : faces)
        if (r.area() > faceRect.area())
            faceRect = r;

    // smoothing
    if (hasLastFace) {
        float a = 0.7f;
        faceRect.x = a*lastFace.x + (1-a)*faceRect.x;
        faceRect.y = a*lastFace.y + (1-a)*faceRect.y;
        faceRect.width  = a*lastFace.width  + (1-a)*faceRect.width;
        faceRect.height = a*lastFace.height + (1-a)*faceRect.height;
    }

    lastFace = faceRect;
    hasLastFace = true;

    int margin = faceRect.width * 0.25;
    cv::Rect roi(
        faceRect.x - margin,
        faceRect.y - margin,
        faceRect.width  + margin*2,
        faceRect.height + margin*2
        );

    roi &= cv::Rect(0,0,mat.cols,mat.rows);

    if (roi.width < 100 || roi.height < 100)
        return;

    cv::Mat face = mat(roi).clone();

    emit faceReady(face);
    emit faceRectReady(QRect(roi.x, roi.y, roi.width, roi.height));
}

void FaceDetectionWorker::setFrame(const QImage &frame)
{
    QMutexLocker locker(&mutex);
    latestFrame = frame.copy();   // guarda apenas o mais recente
}
