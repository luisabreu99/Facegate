#include "camerathread.h"
#include <QDebug>

CameraThread::CameraThread(QObject *parent) : QThread(parent) {}

CameraThread::~CameraThread() {
    stopCamera();
    wait();
}

bool CameraThread::startCamera(int index) {
    if (running) return true;

    if (cap.open(index, cv::CAP_DSHOW)) {   // 👈 FORÇA DirectShow
        running = true;
        start();
        return true;
    }

    qDebug() << "Falha ao abrir camera no indice" << index;
    return false;
}


void CameraThread::stopCamera() {
    running = false;
    if (cap.isOpened()) cap.release();
}

void CameraThread::run() {
    while (running) {
        cv::Mat frame;
        cap >> frame;

        if (!frame.empty()) {
            lastFrame = frame.clone();
            emit frameReady(frame);
        }

        QThread::msleep(30);
    }
}

cv::Mat CameraThread::getLastFrame() const {
    return lastFrame.clone();
}
