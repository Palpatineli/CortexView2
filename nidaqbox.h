#ifndef NIDAQSIGNALSTREAM_H
#define NIDAQSIGNALSTREAM_H

#include <QObject>
#include <QVector>

#ifndef CVICALLBACK
#if defined(__linux__) || defined(__APPLE__)
#define CVICALLBACK
#else
#define CVICALLBACK     __cdecl
#endif  // __linux__ || __APPLE__
#endif  // CVICALLBACK

typedef void *TaskHandle;
typedef signed long int32;
class RecordParams;

class NIDaqBox : public QObject {
    Q_OBJECT
  public:
    explicit NIDaqBox(QObject *parent = 0);
    ~NIDaqBox();

    bool isReady();

  signals:
    void raiseError(QString errorMsg);
    void yieldCameraSignal(int timestamp, int computer_time);
    void yieldDiodeSignal(int timestamp, double diode_signal);

  public slots:
    void init();

  private:
    const quint32 NI_FREQ = 1000;

    // running status
    bool is_initialized;
    RecordParams *params;

    // to read and record analog input
    bool cam_is_high, diode_is_rising;
    double previous_diode, diode_baseline;
    int current_time, last_diode_signal_time;
    double temp_buffer[2];

    // record the diode input, cam input will be recorded by the video stream guys
    TaskHandle hTask;
    void readSample(TaskHandle hTask);

    void onError();
    void lookupError();
    void cleanup();

    friend int32 CVICALLBACK signalEventCallback(TaskHandle, int32, /*uInt32, */void *);
};

int32 CVICALLBACK signalEventCallback(TaskHandle hTask, int32 eventType, /*uInt32 nSamples, */void *callbackData);

#endif // NIDAQSIGNALSTREAM_H
