#ifndef CAPTURASTEP_H
#define CAPTURASTEP_H

#include <QObject>
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <QNetworkAccessManager>

class CameraThread;

namespace Ui {
class MainWindow;   // vamos usar a UI da MainWindow (QStackedWidget)
}

class CapturaStep : public QObject
{
    Q_OBJECT

public:
    explicit CapturaStep(Ui::MainWindow *ui,
                         CameraThread *camera,
                         cv::dnn::Net &net,
                         QObject *parent = nullptr);

    // Chamado quando entras no step de captura
    void startStep(const QString &nome,
                   const QString &apelido,
                   const QString &dataNascimento);

    // Chamado quando sais do step
    void stopStep();

private slots:
    void onFrameReceived(const cv::Mat &frame);
    void onBtnCapturarClicked();

private:
    Ui::MainWindow *ui;
    CameraThread *camera;
    cv::dnn::Net mobileFaceNet;

    cv::CascadeClassifier faceCascade;
    bool capturando = false;
    int contadorFotos = 0;

    QString nome;
    QString apelido;
    QString dataNascimento;
    QString pastaColab;
    int colaboradorId = -1;

    QNetworkAccessManager *http;

    // Funções auxiliares (as mesmas que já tens)
    QImage matToQImage(const cv::Mat &mat);
    cv::Mat preprocessFace(const cv::Mat &face);
    cv::Mat getFaceEmbedding(const cv::Mat &face);
    void enviarColaboradorParaLaravel();

};

#endif // CAPTURASTEP_H
