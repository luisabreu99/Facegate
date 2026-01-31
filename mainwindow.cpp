#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QImage>
#include <QPixmap>
#include <QRegion>

MainWindow::MainWindow(QWidget *parent, const cv::dnn::Net& net)
    : QMainWindow(parent),
    ui(new Ui::MainWindow),
    faceNet(net)
{
    ui->setupUi(this);
    QRegion ellipse(ui->frameCamera->rect(), QRegion::Ellipse);
    ui->frameCamera->setMask(ellipse);
    // Inicializa a câmara
    camera = new CameraThread(this);

    connect(camera, &CameraThread::frameReady,
            this, &MainWindow::onCameraFrame);

    reconhecimentoStep = new ReconhecimentoStep(
        ui->labelVideo_2,
        ui->labelStatus,
        ui->circularProgress,
        camera,
        faceNet,
        this
        );
    // Página inicial = Home
    ui->stackedWidget->setCurrentWidget(ui->pageHome);

    // Step inicial do criar colaborador = Dados
    ui->stackedCriar->setCurrentWidget(ui->stepDados);
}

MainWindow::~MainWindow()
{
    camera->stopCamera();
    delete ui;
}

// Botão: Criar Colaborador (Home)
void MainWindow::on_btnCriarColaborador_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->pageCriarColaborador);
    ui->stackedCriar->setCurrentWidget(ui->stepDados);
}

// Botão: Capturar (Home) → Reconhecimento (pageCaptura)
// Botão: Capturar (Home) → Reconhecimento (pageCaptura)
void MainWindow::on_btnCapturar_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->pageCaptura);
    ui->labelStatus->setText("A iniciar câmara...");

    bool ok = false;

    for (int i = 0; i < 5; ++i) {
        ok = camera->startCamera(i);
        qDebug() << "Tentando abrir camera" << i << ":" << ok;
        if (ok) break;
    }

    if (!ok) {
        ui->labelStatus->setText("❌ Nenhuma câmara encontrada");
        return;
    }

    reconhecimentoStep->start();  // começa reconhecimento
}

// Botão: Seguinte (Step Dados)
void MainWindow::on_btnSeguinte_clicked()
{
    const QString nome    = ui->inputNome->text().trimmed();
    const QString apelido = ui->inputApelido->text().trimmed();
    const QString dataN   = ui->inputDataNascimento->date().toString("yyyyMMdd");

    if (nome.isEmpty() || apelido.isEmpty()) {
        QMessageBox::warning(this, "Dados em falta",
                             "Preencha Nome e Apelido antes de continuar.");
        return;
    }

    nomeTemp = nome;
    apelidoTemp = apelido;
    dataTemp = dataN;

    ui->stackedCriar->setCurrentWidget(ui->stepCaptura);
}

// Botão: Voltar (na pageCaptura)
void MainWindow::on_btnVoltar_clicked()
{
    reconhecimentoStep->stop();
    camera->stopCamera();
    ui->stackedWidget->setCurrentWidget(ui->pageHome);
}
void MainWindow::on_btnVoltar_2_clicked()
{

    ui->stackedWidget->setCurrentWidget(ui->pageHome);
}
void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    QRegion ellipse(ui->frameCamera->rect(), QRegion::Ellipse);
    ui->frameCamera->setMask(ellipse);
}


// Slot que recebe os frames da câmara
void MainWindow::onCameraFrame(const cv::Mat &frame)
{
    if (ui->stackedWidget->currentWidget() != ui->pageCaptura)
        return;

    if (frame.empty()) return;

    cv::Mat rgb;
    cv::cvtColor(frame, rgb, cv::COLOR_BGR2RGB);

    QImage img(rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888);
    ui->labelVideo_2->setPixmap(
        QPixmap::fromImage(img).scaled(
            ui->labelVideo_2->size(),
            Qt::KeepAspectRatioByExpanding,
            Qt::SmoothTransformation
            )
        );
}

