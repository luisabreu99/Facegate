#ifndef RECONHECIMENTODIALOG_H
#define RECONHECIMENTODIALOG_H

#include <QDialog>
#include <QTimer>
#include <QElapsedTimer>
#include <QElapsedTimer>

#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <QSoundEffect>

#include "database.h"

namespace Ui {
class ReconhecimentoDialog;
}

class ReconhecimentoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ReconhecimentoDialog(QWidget *parent, cv::dnn::Net &net);
    ~ReconhecimentoDialog();

protected:
    void showEvent(QShowEvent *event) override;
    // inicia webcam quando janela abrir

private slots:
    void atualizarFrame();

private:
    Ui::ReconhecimentoDialog *ui;
    QTimer *timer;
    cv::VideoCapture cap;
    cv::CascadeClassifier faceCascade;   // HaarCascade para detetar rosto
    QSoundEffect soundRecognized;
    QSoundEffect soundUnrecognized;

    // Modelo ONNX (MobileFaceNet ou ArcFace)
    cv::dnn::Net mobileFaceNet;

    // --- Scan Effect ---
    int scanY = 0;              // posição da barra
    int scanDirection = 1;      // 1 = descendo, -1 = subindo
    int scanSpeed = 2;          // velocidade em px/frame
    bool scanning = false;
    bool jaReconhecido = false;     // se está a executar o scan
    QElapsedTimer scanTimer;    // para controlar tempo
    void enviarRegisto(int userId, const QString &tipo);

    // Funções auxiliares
    QImage matToQImage(const cv::Mat &mat);
    cv::Mat preprocessFace(const cv::Mat &face);        // resize + normalização
    cv::Mat getFaceEmbedding(const cv::Mat &face);      // gera vetor 128/512D

    int reconhecerColaborador(const cv::Mat &faceDescriptor);
};

#endif // RECONHECIMENTODIALOG_H
