#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QImage>
#include <unordered_map>
#include <QDateTime>
#include <opencv2/core.hpp>
#include <array>
#include <face_mesh.hpp>
#include "cameraworker.h"
#include "facedetectionworker.h"
#include "faceembedder.h"
#include "facedatabase.h"
#include "databasemanager.h"
#include "attendanceservice.h"
#include "facemeshworker.h"
#include "apiservice.h"


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_btnCapturar_clicked();
    void on_btnCriarColab_clicked();
    void on_btnVoltar_clicked();
    void on_btnFormContinuar_clicked();
    void on_btnFormVoltar_clicked();
    void onLandmarksReady(
        const std::array<cv::Point3f,
                         CLFML::FaceMesh::NUM_OF_FACE_MESH_POINTS> &points);
    void onFrameProcessed(const QImage &img);
    void onFaceDetected(const QImage &faceROI);
    void onEmbeddingReady(const std::vector<float> &embedding);

signals:
    void frameForDetection(const QImage &frame);
    void faceForEmbedding(const cv::Mat &face);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    Ui::MainWindow *ui;

    // THREADS
    QThread *cameraThread = nullptr;
    QThread *detectionThread = nullptr;
    QThread *embedderThread = nullptr;

    // WORKERS
    CameraWorker *cameraWorker = nullptr;
    FaceDetectionWorker *faceDetectionWorker = nullptr;
    FaceEmbedder *faceEmbedder = nullptr;

    // DATABASE
    FaceDatabase faceDatabase;
    bool databaseLoaded = false;

    DatabaseManager databaseManager;
    AttendanceService *attendanceService = nullptr;
    // RECONHECIMENTO
    std::unordered_map<int, QDateTime> lastRegisterTime;
    const int REGISTER_COOLDOWN = 30;

    // CRIAÇÃO DE COLABORADOR
    std::vector<std::vector<float>> collectedEmbeddings;
    const int REQUIRED_EMBEDDINGS = 5;
    bool extractingFace = false;

    // FRAME ATUAL
    QImage lastFrame;

    // ESTADO APP
    enum class AppMode {
        Idle,
        Recognition,
        Creating
    };
    AppMode appMode = AppMode::Idle;

    // FUNÇÕES PRIVADAS
    void startCameraPipeline();
    void stopCameraPipeline();
    void finalizeColaborador();
    QThread *meshThread = nullptr;
    FaceMeshWorker *faceMeshWorker = nullptr;



    std::vector<QPointF> targetLandmarks;   // vem do FaceMesh (5 FPS)
    std::vector<QPointF> drawLandmarks;     // desenhado a 30 FPS
    QRect lastFaceRect;
    bool faceVisible = false;
    QElapsedTimer faceLostTimer;

    ApiService *apiService;
};

#endif
