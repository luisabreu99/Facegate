#ifndef FACEDETECTIONWORKER_H
#define FACEDETECTIONWORKER_H

#include <QObject>
#include <QImage>
#include <QMutex>
#include <QTimer>
#include <opencv2/opencv.hpp>

class FaceDetectionWorker : public QObject
{
    Q_OBJECT

public:
    explicit FaceDetectionWorker(QObject *parent = nullptr);

public slots:
    void setFrame(const QImage &frame);   // ⭐ novo slot
    void process();
    void start();     // ⭐ loop interno

signals:
    void faceReady(const cv::Mat &face);
    void faceRectReady(const QRect &rect);

private:
    cv::CascadeClassifier faceCascade;

    // ⭐ latest frame buffer
    QImage latestFrame;
    QMutex mutex;

    QTimer *timer;

    cv::Rect lastFace;
    bool hasLastFace = false;
};

#endif
