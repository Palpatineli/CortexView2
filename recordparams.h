#ifndef RECORDPARAMS_H
#define RECORDPARAMS_H

#include <QString>
#include <QMutex>

class RecordParams
{
  public:
    static RecordParams *getParams();

    QString getFilePath();
    void setFilePath(const QString &value);

    int getPeriodInFrames() const;
    void setPeriodInFrames(const int value);

    double getPeriodInSeconds() const;
    void setPeriodInSeconds(double value);

    int getCycleNo() const;
    void setCycleNo(const int value);

    bool lock();
    void unlock();
    bool isLocked() const;

    int getExposureTime() const;
    void setExposureTime(int value);

    int getFramePerPulse() const;
    void setFramePerPulse(int value);

private:
    RecordParams();
    RecordParams(RecordParams const&) = delete;             // C++11
    void operator=(RecordParams const&) = delete;           // C++11

    QString file_path;
    QMutex file_path_lock;
    int period_in_frames, exposure_time, frame_per_pulse;
    double period_in_seconds;
    int cycle_no;

    bool is_locked;
    QMutex lock_lock;
};

#endif // RECORDPARAMS_H
