#include <QSettings>
#include <QFileDialog>
#include <QDir>
#include <QVector>
#include <QTimer>
#include <QDebug>

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "recordparams.h"

typedef QVector<quint16> ImageArray;
typedef QVector<int> IntArray;
typedef QVector<double> DoubleArray;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent), ui(new Ui::MainWindow),
    save_thread(NULL), save_worker(NULL) {
    qRegisterMetaType<ImageArray>("ImageArray");
    qRegisterMetaType<IntArray>("IntArray");
    qRegisterMetaType<DoubleArray>("DoubleArray");

    params = RecordParams::getParams();
    ui->setupUi(this);
    ui->filePathEdit->setText(params->getFilePath());
    ui->cycleNoSpin->setValue(params->getCycleNo());
    ui->periodSpinInF->setValue(params->getPeriodInFrames());
    ui->periodSpinInS->setValue(params->getPeriodInSeconds());

    QCoreApplication::setOrganizationName("SurLabMIT");
    QCoreApplication::setApplicationName("CortexView2");
    QSettings::setDefaultFormat(QSettings::IniFormat);

    niDaq_box.moveToThread(&niDaq_thread);
    connect(&niDaq_box, SIGNAL(raiseError(QString)), &error_dialog, SLOT(showMessage(QString)));
    connect(&niDaq_thread, SIGNAL(started()), &niDaq_box, SLOT(init()));

    camera.moveToThread(&camera_thread);
    connect(&camera, SIGNAL(raiseError(QString)), &error_dialog, SLOT(showMessage(QString)));
    connect(&camera_thread, SIGNAL(started()), &camera, SLOT(init()));

    processor.moveToThread(&processing_thread);
    connect(&processor, SIGNAL(raiseError(QString)), &error_dialog, SLOT(showMessage(QString)));

    connect(&camera, SIGNAL(yieldFrame(int,ImageArray)), &processor, SLOT(pushImage(int,ImageArray)));
    connect(&niDaq_box, SIGNAL(yieldCameraSignal(int,int)), &processor, SLOT(pushCameraSignal(int,int)));
    connect(&niDaq_box, SIGNAL(yieldDiodeSignal(int,double)), &processor, SLOT(pushDiodeSignal(int,double)));

    connect(&processor, SIGNAL(yieldImage(QImage)), ui->cameraView, SLOT(updateImage(QImage)));

    // actions
    // cameraView drags ROI
    connect(ui->cameraView, SIGNAL(roiChanged()), &camera, SLOT(setROI()));
    // save picture
    connect(this, SIGNAL(takePicture(QString)), &processor, SLOT(takePicture(QString)));
    // start recording
    connect(this, SIGNAL(stopRecording()), &processor, SLOT(stopRecording()));
    // close MainWindow
    connect(this, SIGNAL(closeSignal()), &camera_thread, SLOT(quit()));
    connect(this, SIGNAL(closeSignal()), &processing_thread, SLOT(quit()));
    connect(this, SIGNAL(closeSignal()), &niDaq_thread, SLOT(quit()));
    // progress bar
    connect(&processor, SIGNAL(reportRemainingTime(int)), ui->cameraView, SLOT(updateRemainingTime(int)));

    niDaq_thread.start();
    camera_thread.start();
    processing_thread.start();
}

MainWindow::~MainWindow() {
    delete ui;
}

bool MainWindow::verifyInput() {
    QDir file_path(ui->filePathEdit->text());
    if (!file_path.cdUp()) {
        error_dialog.showMessage(tr("Directory does not Exit!\nPlease re-enter your save file folder."));
        return false;
    }
    return true;
}

void MainWindow::startRecording() {
    // ui operations
    ui->cameraButton->setDisabled(true);
    ui->recordButton->setChecked(true);

    // setup the save thread
    save_worker = new SaveWorker();
    save_thread = new QThread();
    save_worker->moveToThread(save_thread);
    connect(&processor, SIGNAL(finishSaving()), save_worker, SLOT(stop()));

    connect(save_thread, SIGNAL(started()), save_worker, SLOT(start()));
    connect(save_worker, SIGNAL(raiseError(QString)), &error_dialog, SLOT(showMessage(QString)));
    connect(save_worker, SIGNAL(started()), &processor, SLOT(startRecording()));

    // schedule its death
    connect(save_worker, SIGNAL(finished()), save_worker, SLOT(deleteLater()));
    connect(save_worker, SIGNAL(destroyed()), save_thread, SLOT(quit()));
    connect(save_thread, SIGNAL(finished()), save_thread, SLOT(deleteLater()));

    // end the recording when done
    connect(save_worker, SIGNAL(destroyed()), this, SLOT(recordingFinished()));

    connect(this, SIGNAL(closeSignal()), save_worker, SLOT(stop()));

    // saving paths
    connect(&processor, SIGNAL(yieldImageData(int,ImageArray)), save_worker, SLOT(pushFrame(int,ImageArray)));
    connect(&processor, SIGNAL(saveDoubleSeries(DoubleArray,QByteArray)), save_worker, SLOT(saveDoubleSeries(DoubleArray,QByteArray)));
    connect(&processor, SIGNAL(saveIntSeries(IntArray,QByteArray)), save_worker, SLOT(saveIntSeries(IntArray,QByteArray)));

    save_thread->start();
    ui->cameraView->updateStatus("REC", Qt::red);
}

void MainWindow::recordingFinished() {
    save_thread = NULL;
    save_worker = NULL;
    ui->cameraView->updateStatus("Standby", Qt::green);
    ui->cameraButton->setEnabled(true);
    ui->recordButton->setChecked(false);
}

void MainWindow::closeEvent(QCloseEvent *event) {
    emit(closeSignal());
    if (save_worker) save_thread->wait(100);
    camera_thread.wait(200);
    processing_thread.wait(100);
    niDaq_thread.wait(200);
    QMainWindow::closeEvent(event);
}

void MainWindow::on_recordButton_released() {
    if (!(ui->recordButton->isChecked())) {emit(stopRecording()); return;}
    if (!(camera.isReady() && niDaq_box.isReady())) {error_dialog.showMessage("devices not ready!"); return;}
    qDebug() << "camera and nidaq are both ready";
    if (params->isLocked()) {error_dialog.showMessage("Already Recording!"); return;}
    if (!verifyInput()) return;
    params->setPeriodInFrames(ui->periodSpinInF->value());
    params->setPeriodInSeconds(ui->periodSpinInS->value());
    params->setCycleNo(ui->cycleNoSpin->value());
    params->setFilePath(ui->filePathEdit->text());
    params->setFramePerPulse(ui->framePerPulse->value());
    params->lock();
    qDebug() << "recordparams locked";
    startRecording();
}

void MainWindow::on_cameraButton_released() {
    if (!(camera.isReady() && niDaq_box.isReady())) {error_dialog.showMessage("devices not ready!"); return;}
    qDebug() << "take a picture";
    emit(takePicture(ui->filePathEdit->text()));
}

void MainWindow::on_browseFileButton_released() {
    QString file_path = QFileDialog::getSaveFileName(this, tr("Open Image"), ui->filePathEdit->text(), tr("Data Files (*.h5);;Single Image (*.png)"));
    ui->filePathEdit->setText(file_path);
}

void MainWindow::on_resetROIButton_released() {
    ui->cameraView->resetROI();
}

void MainWindow::on_binningButton_released() {
    bool bin_status = ui->cameraView->toggleBin(ui->binningButton->isChecked());
    ui->binningButton->setChecked(bin_status);
}
