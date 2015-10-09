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

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

signals:
    void takePicture(QString file_path);
    void closeSignal();

private slots:
    void onBrowse();
    void onRecord();
    void onTakePicture();
    void recordingFinished();
    void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;

private:
    Ui::MainWindow *ui;

    void startRecording();
    QErrorMessage error_dialog;
    RecordParams* params;

    QThread* save_thread;
    SaveWorker* save_worker;

    QThread niDaq_thread;
    NIDaqBox niDaq_box;

    QThread camera_thread;
    Camera camera;
    QTimer temp_check_timer;

    QThread processing_thread;
    OnlineProcessor processor;

    bool verify_input();
};

#endif // MAINWINDOW_H
