#ifndef CAMERATHREAD_H
#define CAMERATHREAD_H

#include <QThread>
#include <opencv2/opencv.hpp>

class CameraThread : public QThread
{
    Q_OBJECT
public:
    explicit CameraThread(QObject *parent = nullptr);
    ~CameraThread();

    bool startCamera(int index = 0);
    void stopCamera();

    cv::Mat getLastFrame() const;   // 👈 NOVA FUNÇÃO

signals:
    void frameReady(const cv::Mat &frame);

protected:
    void run() override;

private:
    cv::VideoCapture cap;
    bool running = false;
    cv::Mat lastFrame;              // 👈 NOVO
};

#endif // CAMERATHREAD_H
