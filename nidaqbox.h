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

typedef void* TaskHandle;
typedef signed long int32;
class RecordParams;

class NIDaqBox : public QObject
{
    Q_OBJECT
public:
    explicit NIDaqBox(QObject *parent = 0);
    ~NIDaqBox();

    bool isReady();

signals:
    void raiseError(QString errorMsg);
    void cameraFrameReady(quint64 timestamp, double phase);
    void yieldDiodeSignal(quint64 timestamp, double diode_signal);

public slots:
    void init();
    void reset();

private:
    const quint32 NI_FREQ = 1000;
    const quint32 frames_per_pulse = 6;

    // running status
    bool is_initialized;
    RecordParams* params;

    // to read and record analog input
    bool cam_is_low;
    int32 read_no;
    double tempBuffer[2];
    QVector<double> diode_levels;  // signal levels to digitize diode signals
    double previous_diode, previous_diode_diff, diode_diff;
    bool diode_rising;
    quint64 current_time, current_diode_signal_time, last_diode_signal_time;
    double current_phase, last_diode_signal_phase, diode_onset_level;

    // to calculate current_phase
    double phase_per_tick, phase_per_pulse;

    // record the diode input, cam input will be recorded by the video stream guys
    TaskHandle hTask;
    void readSample(TaskHandle hTask);

    void onError();
    void lookupError();
    void cleanup();

    friend int32 CVICALLBACK signalEventCallback(TaskHandle, int32, /*uInt32, */void*);
};

int32 CVICALLBACK signalEventCallback(TaskHandle hTask, int32 eventType, /*uInt32 nSamples, */void *callbackData);

#endif // NIDAQSIGNALSTREAM_H
