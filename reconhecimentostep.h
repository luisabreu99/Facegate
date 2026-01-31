#ifndef RECONHECIMENTOSTEP_H
#define RECONHECIMENTOSTEP_H

#include <QObject>
#include <QLabel>
#include <QElapsedTimer>
#include <QSoundEffect>

#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>

#include "circularprogress.h"
#include "camerathread.h"
#include "database.h"
#include "facelandmarks.h"
#include <memory>


class ReconhecimentoStep : public QObject
{
    Q_OBJECT
public:
    explicit ReconhecimentoStep(QLabel *videoLabel,
                                QLabel *statusLabel,
                                CircularProgress *progress,
                                CameraThread *camera,
                                cv::dnn::Net &net,
                                QObject *parent = nullptr);

    void start();
    void stop();

private slots:
    void onFrame(const cv::Mat &frame);

private:
    QLabel *labelVideo;
    QLabel *labelStatus;
    CircularProgress *progress;
    CameraThread *camera;

    // Redes neurais
    cv::dnn::Net mobileFaceNet;     // embeddings


    cv::CascadeClassifier faceCascade;

    // Landmarks
    std::unique_ptr<IFaceLandmarks> faceLandmark;


    // Scan + progresso
    int meioCiclos = 0;
    const int totalMeioCiclos = 5;
    int scanY = 0;
    int scanDirection = 1;
    int scanSpeed = 20;
    bool scanning = false;
    bool jaReconhecido = false;
    QElapsedTimer scanTimer;

    // Sons
    QSoundEffect soundRecognized;
    QSoundEffect soundUnrecognized;

    // Funções utilitárias
    QImage matToQImage(const cv::Mat &mat);
    cv::Mat preprocessFace(const cv::Mat &face);
    cv::Mat getFaceEmbedding(const cv::Mat &face);
    int reconhecerColaborador(const cv::Mat &faceDescriptor);
    void enviarRegisto(int userId, const QString &tipo);
};

#endif
