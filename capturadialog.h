#ifndef CAPTURADIALOG_H
#define CAPTURADIALOG_H

#include <QDialog>
#include <QTimer>
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <QNetworkAccessManager>
// -----------------------------
// CapturaDialog
// -----------------------------
namespace Ui {
class CapturaDialog;
}

class CapturaDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CapturaDialog(const QString &nome,
                           const QString &apelido,
                           const QString &dataNascimento,
                           cv::dnn::Net &net,
                           QWidget *parent = nullptr);
    ~CapturaDialog();
protected:
    void showEvent(QShowEvent *event) override;   // 👈 inicia webcam só quando a janela aparecer

private slots:
    void atualizarFrame();
    void on_btnCapturar_clicked();

private:
    Ui::CapturaDialog *ui;
    QTimer *timer;
    cv::VideoCapture cap;
    cv::CascadeClassifier faceCascade;   // HaarCascade só para localizar rosto
    bool capturando = false;
    int contadorFotos = 0;

    QString nome;
    QString apelido;
    QString dataNascimento;
    QString pastaColab;
    int colaboradorId = -1;   // id guardado na BD

    QImage matToQImage(const cv::Mat &mat);

    // --- MobileFaceNet (via OpenCV DNN) ---
    cv::dnn::Net mobileFaceNet;   // rede carregada a partir do .onnx

    // Funções auxiliares
    cv::Mat preprocessFace(const cv::Mat &face);   // prepara rosto (112x112, RGB, normalizado)
    cv::Mat getFaceEmbedding(const cv::Mat &face); // extrai vetor 128D (ou 512D dependendo do modelo)
    QNetworkAccessManager *http;  // 👈 adiciona isto
    void enviarColaboradorParaLaravel();
};

#endif // CAPTURADIALOG_H
