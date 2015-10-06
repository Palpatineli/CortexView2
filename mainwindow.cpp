#include <QSettings>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDir>
#include <QTimer>

#include "sessiondialog.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "recordparams.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent), ui(new Ui::MainWindow), is_recording(false)
{
    params = RecordParams::getParams();
    ui->setupUi(this);
    ui->recordButton->setCheckable(true);

    QCoreApplication::setOrganizationName("SurLabMIT");
    QCoreApplication::setApplicationName("KejiLi");
    QSettings::setDefaultFormat(QSettings::IniFormat);


    niDaqBox.moveToThread(&niDaqThread);
    connect(&niDaqThread, SIGNAL(started()), &niDaqBox, SLOT(init()));
    connect(&niDaqThread, SIGNAL(finished()), &niDaqBox, SLOT(deleteLater()));
    connect(&niDaqBox, SIGNAL(yieldDiodeSignal(quint64,double)), &saveWorker, SLOT(pushDiodeSignal(quint64,double)));
    connect(&niDaqBox, SIGNAL(raiseError(QString)), &errorDialog, SLOT(showMessage(QString)));

    camera.moveToThread(&cameraThread);
    connect(&cameraThread, SIGNAL(started()), &camera, SLOT(init()));
    connect(&cameraThread, SIGNAL(finished()), &camera, SLOT(deleteLater()));
    connect(&camera, SIGNAL(yieldFrame(quint64,double,QVector2D<quint16>)), &saveWorker, SLOT(pushFrame(quint64,double,QVector2D<quint16>)));
    connect(&camera, SIGNAL(yieldFrame(quint64,double,QVector2D<quint16>)), this, SLOT(updateImage(quint64,double,QVector2D<quint16>)));

    niDaqThread.start();
    cameraThread.start();
    saveThread.start();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::openRecordDialog() {
    SessionDialog recordingDialog(params, this);
    if (QDialog::Accepted == recordingDialog.exec()) {
        recordingDialog.getRecordingParams();
        startRecord();
    }
}

void MainWindow::openTakePictureDialog()
{
    QDir file_folder(params->file_path);
    file_folder.cdUp();
    QString file_path = QFileDialog::getSaveFileName(this, tr("Open Image"), file_folder.filePath("Data.tiff"), tr("Data Files (*.tiff)"));
    emit(savePicture(file_path));
}

void MainWindow::startRecord()
{
    if (!params->lock()) return;
    if (!saveWorker.isNull() || !saveThread.isNull()) return;
    saveWorker = new SaveWorker(params);
    saveThread = new QThread();
    saveWorker->moveToThread(saveThread);
    connect(&camera, SIGNAL(yieldFrame(const quint64,const double,QSharedPointer<QVector<quint16>>,QSharedPointer<QRect>)),
            saveWorker, SLOT(pushFrame(const quint64,const double,QSharedPointer<QVector<quint16>>,QSharedPointer<QRect>)));
    connect(saveThread, SIGNAL(started()), saveWorker, SLOT(start()));
    connect(saveWorker, SIGNAL(finished()), saveThread, SLOT(quit()));
    connect(saveWorker, SIGNAL(finished()), saveWorker, SLOT(deleteLater()));
    connect(saveThread, SIGNAL(finished()), saveThread, SLOT(deleteLater()));
    connect(saveThread, SIGNAL(finished()), this, SLOT(recordingFinished()));
    ui->cameraButton->setDisabled(true);
    ui->recordButton->setChecked(true);
    disconnect(ui->recordButton, SIGNAL(released()), this, SLOT(startRecord()));
    connect(ui->recordButton, SIGNAL(released()), saveWorker, SLOT(end()));
    saveThread->start();
    is_recording = true;
}

void MainWindow::recordingFinished() {
    ui->cameraButton->setEnabled(true);
    ui->recordButton->setChecked(false);
    is_recording = false;
}
