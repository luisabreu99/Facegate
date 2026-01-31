#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <opencv2/dnn.hpp>
#include "camerathread.h"
#include "reconhecimentostep.h"
#include "circularprogress.h"
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr,
                        const cv::dnn::Net& net = cv::dnn::Net());
    ~MainWindow();
protected:
     void resizeEvent(QResizeEvent *event) override;
private slots:
    void on_btnCriarColaborador_clicked();
    void on_btnCapturar_clicked();
    void on_btnSeguinte_clicked();
    void on_btnVoltar_clicked();
    void on_btnVoltar_2_clicked();

    void onCameraFrame(const cv::Mat &frame);   // recebe vídeo da webcam

private:
    Ui::MainWindow *ui;
    cv::dnn::Net faceNet;
    ReconhecimentoStep *reconhecimentoStep;
    // Dados temporários do wizard
    QString nomeTemp;
    QString apelidoTemp;
    QString dataTemp;

    // Câmara partilhada
    CameraThread *camera;
};

#endif // MAINWINDOW_H
