#include "facemeshworker.h"
#include <QDebug>

FaceMeshWorker::FaceMeshWorker(QObject *parent)
    : QObject(parent)
{
    mesh.load_model(
        "/home/abreu/Projetos/Face_Mesh.Cpp/models/CPU/face_mesh.tflite"
        );

    initialized = true;
    qDebug() << "FaceMesh carregado com sucesso";
}

void FaceMeshWorker::processFrame(const cv::Mat &frame)
{
    if (!initialized || frame.empty())
        return;

    // ⭐ LIMITADOR FPS FACEMESH (CRÍTICO!)
    if (!meshTimer.isValid())
        meshTimer.start();

    if (meshTimer.elapsed() < 70)   // ~14 FPS
        return;

    meshTimer.restart();

    cv::Mat safe = frame.clone();
    cv::resize(safe, safe, cv::Size(192,192));

    mesh.load_image(safe);
    auto points = mesh.get_face_mesh_points();

    emit landmarksReady(points);
}

