#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QErrorMessage>
#include <QThread>
#include <QReadWriteLock>
#include <QTimer>
#include "saveworker.h"
#include "nidaqbox.h"
#include "nidaqbox.h"
#include "camera.h"
#include "onlineprocessor.h"

class RecordParams;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

  public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

  signals:
    void takePicture(QString file_path);
    void closeSignal();
    void stopRecording();

  private slots:
    void recordingFinished();
    void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;

    void on_recordButton_released();
    void on_cameraButton_released();
    void on_browseFileButton_released();
    void on_resetROIButton_released();
    void on_binningButton_released();

private:
    Ui::MainWindow *ui;

    void startRecording();
    QErrorMessage error_dialog;
    RecordParams *params;

    QThread *save_thread;
    SaveWorker *save_worker;

    QThread niDaq_thread;
    NIDaqBox niDaq_box;

    QThread camera_thread;
    Camera camera;

    QThread processing_thread;
    OnlineProcessor processor;

    bool verifyInput();
};

#endif // MAINWINDOW_H
