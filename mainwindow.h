#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QErrorMessage>
#include <QThread>
#include <QReadWriteLock>
#include "saveworker.h"
#include "nidaqbox.h"
#include "nidaqbox.h"
#include "camera.h"
#include "onlineprocessor.h"

class RecordParams;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

signals:
    void initSaving(QString file_path, int frame_no);
    void savePicture(QString file_path);

private slots:
    void resetROI();
    void openRecordDialog();
    void openTakePictureDialog();

    void recordingFinished();

private:
    Ui::MainWindow *ui;

    void startRecord();
    void takePicture();
    void init();
    QErrorMessage errorDialog;
    QSharedPointer<RecordParams> params;

    QSharedPointer<QThread> saveThread;
    QSharedPointer<SaveWorker> saveWorker;

    QThread niDaqThread;
    NIDaqBox niDaqBox;
    NIDaqBox niDaqBox;

    bool is_recording;
    QThread cameraThread;
    Camera camera;

    QThread processingThread;
    OnlineProcessor processor;
};

#endif // MAINWINDOW_H
