#ifndef CAMERAWORKER_H
#define CAMERAWORKER_H

#include <QObject>
#include <QImage>
#include <QTimer>
#include <opencv2/opencv.hpp>

class CameraWorker : public QObject
{
    Q_OBJECT

public:
    explicit CameraWorker(QObject *parent = nullptr);
    ~CameraWorker();

public slots:
    void startCamera();
    void stopCamera();

signals:
    void frameReady(const QImage &frame);

private:
    cv::VideoCapture cap;
    QTimer *timer = nullptr;
    bool running = false;

private slots:
    void grabFrame();
};

#endif
