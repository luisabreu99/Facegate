#ifndef FACEMESHWORKER_H
#define FACEMESHWORKER_H

#include <QObject>
#include <QImage>
#include <opencv2/opencv.hpp>
#include <array>
#include <face_mesh.hpp>
#include <QElapsedTimer>
class FaceMeshWorker : public QObject
{
    Q_OBJECT

public:
    explicit FaceMeshWorker(QObject *parent = nullptr);

public slots:
    void processFrame(const cv::Mat &frame);

signals:
    void landmarksReady(
        const std::array<cv::Point3f,
                         CLFML::FaceMesh::NUM_OF_FACE_MESH_POINTS> &points);

private:
    CLFML::FaceMesh::FaceMesh mesh;
    bool initialized = false;
    QElapsedTimer meshTimer;

};

#endif // FACEMESHWORKER_H
