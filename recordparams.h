#ifndef RECORDPARAMS_H
#define RECORDPARAMS_H

#include <QString>
#include <QReadWriteLock>

class RecordParams
{
  public:
    static RecordParams *getParams();

    QString getFile_path();
    void setFile_path(const QString &value);

    int getPeriod_in_frames();
    void setPeriod_in_frames(int value);

    double getPeriod_in_seconds();
    void setPeriod_in_seconds(double value);

    int getCycle_no();
    void setCycle_no(int value);

    bool lock();
    void unlock();

private:
    RecordParams();
    RecordParams(RecordParams const&) = delete;             // C++11
    void operator=(RecordParams const&) = delete;           // C++11

    QString file_path;
    QReadWriteLock file_path_lock;
    int period_in_frames;
    QReadWriteLock period_in_frames_lock;
    double period_in_seconds;
    QReadWriteLock period_in_seconds_lock;
    int cycle_no;
    QReadWriteLock cycle_no_lock;

    bool is_recording;
    QReadWriteLock recording_lock;
};

#endif // RECORDPARAMS_H
