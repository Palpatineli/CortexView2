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

class NIDaqBox : public QObject
{
    Q_OBJECT
public:
    explicit NIDaqBox(QObject *parent = 0);
    ~NIDaqBox();

signals:
    void raiseError(QString errorMsg);
    void cameraFrameReady(quint64 timestamp, double phase);
    void yieldDiodeSignal(quint64 timestamp, double diode_signal);

public slots:
    void init();
    void setPhaseParams(quint32 frame_no, double cycle_time);

private:
    const quint32 NI_FREQ = 1000;
    // running status
    bool is_initialized;

    // to read and record analog input
    QVector<double> diode_levels;  // signal levels to digitize diode signals
    qint32 error_id, read_no;
    double previous_diode, previous_cam, previous_diode_diff, previous_cam_diff, diode_diff, cam_diff;
    double tempBuffer[2];
    bool diode_rising;
    quint64 current_time, current_diode_signal_time, last_diode_signal_time;
    double current_phase, last_diode_signal_phase, diode_onset_level;

    // to calculate current_phase
    double phase_per_tick, phase_per_pulse;

    // record the diode input, cam input will be recorded by the video stream guys
    TaskHandle hTask;
    void readSample(TaskHandle hTask);

    friend qint32 CVICALLBACK signalEventCallback(TaskHandle, qint32, /*uInt32, */void*);
};

qint32 CVICALLBACK signalEventCallback(TaskHandle hTask, qint32 eventType, /*uInt32 nSamples, */void *callbackData);

#endif // NIDAQSIGNALSTREAM_H
