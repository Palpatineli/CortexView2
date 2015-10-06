#ifndef SAVEWORKER_H
#define SAVEWORKER_H

#include <QObject>
#include <QSharedPointer>
#include <QVector>
#include <QRect>

class RecordParams;
typedef int hid_t;
typedef QVector<quint16> ImageArray;

class SaveWorker : public QObject
{
    Q_OBJECT
public:
    explicit SaveWorker(QRect roi, QObject *parent = 0);

    static bool savePicture(const QString file_path, const QRect roi_in, const ImageArray image);

signals:
    void finished();
    void raiseError(QString error_message);

public slots:
    void pushFrame(const quint64 timestamp, const double frame_phase, ImageArray frame, QRect roi_in);
    void pushDiodeSignal(quint64 timestamp, double signal_size);
    void start();
    void stop();

private:
    // HDF5 related
    hid_t hFile;
    hid_t hFrames, hPTable, hFrameTimestamp, hFramePhase;
    hid_t hDiodes, hDiodeTimestamp, hDiodeSignal;
    hid_t H5T_IMAGE;

    bool is_initialized, needs_cleanUp;
    RecordParams* params;
    QRect roi;
    QVector<quint64> diode_timestamp;
    QVector<double> diode_signal;
    quint64 frame_no;

    void cleanUp();
    void finishSaving();
};

#endif // SAVEWORKER_H
