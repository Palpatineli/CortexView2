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

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent), ui(new Ui::MainWindow),
    save_thread(NULL), save_worker(NULL) {
    qRegisterMetaType<ImageArray>("ImageArray");

    params = RecordParams::getParams();
    ui->setupUi(this);
    ui->filePathEdit->setText(params->getFile_path());
    ui->cycleNoSpin->setValue(params->getCycle_no());
    ui->periodSpinInF->setValue(params->getPeriod_in_frames());
    ui->periodSpinInS->setValue(params->getPeriod_in_seconds());

    QCoreApplication::setOrganizationName("SurLabMIT");
    QCoreApplication::setApplicationName("CortexView2");
    QSettings::setDefaultFormat(QSettings::IniFormat);

    niDaq_box.moveToThread(&niDaq_thread);
    connect(&niDaq_box, SIGNAL(raiseError(QString)), &error_dialog, SLOT(showMessage(QString)));
    connect(&niDaq_thread, SIGNAL(started()), &niDaq_box, SLOT(init()));

    camera.moveToThread(&camera_thread);
    connect(&camera, SIGNAL(raiseError(QString)), &error_dialog, SLOT(showMessage(QString)));
    connect(&camera_thread, SIGNAL(started()), &camera, SLOT(init()));
//    connect(&niDaq_box, SIGNAL(cameraFrameReady(quint64, double)), &camera, SLOT(captureFrame(quint64, double)));

    processor.moveToThread(&processing_thread);
    connect(&processor, SIGNAL(raiseError(QString)), &error_dialog, SLOT(showMessage(QString)));
    connect(&camera, SIGNAL(yieldFrame(quint64, double, ImageArray, QRect)), &processor, SLOT(updateImage(quint64, double, ImageArray, QRect)));
    connect(&processor, SIGNAL(yieldImage(QImage, QRect)), ui->cameraView, SLOT(updateImage(QImage, QRect)));

    // actions
    // browse for save file path
    connect(ui->browseFileButton, SIGNAL(released()), this, SLOT(onBrowse()));
    // resetROI
    connect(ui->resetROIButton, SIGNAL(released()), &camera, SLOT(setROI()));
    connect(ui->resetROIButton, SIGNAL(released()), ui->cameraView, SLOT(setROI()));
    // cameraView drags ROI
    connect(ui->cameraView, SIGNAL(roiChangeEvent(QRect)), &camera, SLOT(setROI(QRect)));
    // update temperature
    connect(&temp_check_timer, SIGNAL(timeout()), &camera, SLOT(checkTemp()));
    connect(&camera, SIGNAL(yieldTemperature(int)), ui->temperatureBar, SLOT(setValue(int)));
    // save picture
    connect(ui->cameraButton, SIGNAL(released()), this, SLOT(onTakePicture()));
    connect(this, SIGNAL(takePicture(QString)), &processor, SLOT(takePicture(QString)));
    // start recording
    connect(ui->recordButton, SIGNAL(released()), this, SLOT(onRecord()));
    // close MainWindow
    connect(this, SIGNAL(closeSignal()), &camera_thread, SLOT(quit()));
    connect(this, SIGNAL(closeSignal()), &processing_thread, SLOT(quit()));
    connect(this, SIGNAL(closeSignal()), &niDaq_thread, SLOT(quit()));

    niDaq_thread.start();
    camera_thread.start();
    processing_thread.start();
    temp_check_timer.start(2000);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::onRecord() {
    if (!(camera.isReady() && niDaq_box.isReady())) {error_dialog.showMessage("devices not ready!"); return;}
    qDebug() << "camera and nidaq are both ready";
    if (!params->lock()) {error_dialog.showMessage("Already Recording!"); return;}
    qDebug() << "recordparams locked";
    if(!verify_input()) return;
    params->unlock();
    params->setPeriod_in_frames(ui->periodSpinInF->value());
    params->setPeriod_in_seconds(ui->periodSpinInS->value());
    params->setCycle_no(ui->cycleNoSpin->value());
    params->setFile_path(ui->filePathEdit->text());
    params->lock();
    startRecording();
}

bool MainWindow::verify_input() {
    QDir file_path(ui->filePathEdit->text());
    if (!file_path.cdUp()) {
        error_dialog.showMessage(tr("Directory does not Exit!\nPlease re-enter your save file folder."));
        return false;
    }
    return true;
}

void MainWindow::onTakePicture() {
    if (!(camera.isReady() && niDaq_box.isReady())) {error_dialog.showMessage("devices not ready!"); return;}
    qDebug() << "take a picture";
    emit(takePicture(ui->filePathEdit->text()));
}

void MainWindow::onBrowse() {
    QString file_path = QFileDialog::getSaveFileName(this, tr("Open Image"), ui->filePathEdit->text(), tr("Data Files (*.h5);;Single Image (*.png)"));
    ui->filePathEdit->setText(file_path);
}

void MainWindow::startRecording() {
    // ui operations
    ui->cameraButton->setDisabled(true);
    ui->recordButton->setChecked(true);

    // setup the save thread
    save_worker = new SaveWorker(camera.getROI());
    save_thread = new QThread();
    save_worker->moveToThread(save_thread);
    disconnect(ui->recordButton, SIGNAL(released()), this, SLOT(onRecord()));
    connect(ui->recordButton, SIGNAL(released()), save_worker, SLOT(stop()));

    connect(save_thread, SIGNAL(started()), save_worker, SLOT(start()));
    connect(save_worker, SIGNAL(raiseError(QString)), &error_dialog, SLOT(showMessage(QString)));
    connect(save_worker, SIGNAL(started()), &niDaq_box, SLOT(reset()));
    connect(save_worker, SIGNAL(started()), &processor, SLOT(startRecording()));

    // schedule its death
    connect(save_worker, SIGNAL(finished()), save_worker, SLOT(deleteLater()));
    connect(save_worker, SIGNAL(destroyed()), save_thread, SLOT(quit()));
    connect(save_worker, SIGNAL(destroyed()), save_thread, SLOT(deleteLater()), Qt::DirectConnection);

    // end the recording when done
    connect(save_worker, SIGNAL(finished()), this, SLOT(recordingFinished()));
    connect(save_worker, SIGNAL(finished()), &niDaq_box, SLOT(reset()));
    connect(save_worker, SIGNAL(finished()), &processor, SLOT(stopRecording()));

    connect(this, SIGNAL(closeSignal()), save_worker, SLOT(stop()));

    // saving paths
    connect(&camera, SIGNAL(yieldFrame(quint64, double, ImageArray, QRect)), save_worker, SLOT(pushFrame(quint64, double, ImageArray, QRect)));
    connect(&niDaq_box, SIGNAL(yieldDiodeSignal(quint64, double)), save_worker, SLOT(pushDiodeSignal(quint64, double)));

    save_thread->start();
    ui->cameraView->updateStatus("REC", Qt::red);
}

void MainWindow::recordingFinished() {
    disconnect(ui->recordButton, SIGNAL(released()), save_worker, SLOT(stop()));
    connect(ui->recordButton, SIGNAL(released()), this, SLOT(onRecord()));
    save_thread = NULL;
    save_worker = NULL;
    ui->cameraView->updateStatus("Standby", Qt::green);
    ui->cameraButton->setEnabled(true);
    ui->recordButton->setChecked(false);
}

void MainWindow::closeEvent(QCloseEvent * event) {
    emit(closeSignal());
    if (save_worker) save_thread->wait(100);
    camera_thread.wait(200);
    processing_thread.wait(100);
    niDaq_thread.wait(200);
    QMainWindow::closeEvent(event);
}
