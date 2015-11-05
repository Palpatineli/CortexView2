#ifndef SAVEWORKER_H
#define SAVEWORKER_H

#include <QObject>
#include <QSharedPointer>
#include <QVector>
#include <QRect>

class RecordParams;
class RegionOfInterest;
typedef int hid_t;
typedef QVector<quint16> ImageArray;
typedef QVector<int> IntArray;
typedef QVector<double> DoubleArray;

class SaveWorker : public QObject {
    Q_OBJECT
  public:
    explicit SaveWorker(QObject *parent = 0);

  signals:
    void started();
    void finished();
    void raiseError(QString error_message);

  public slots:
    void pushFrame(const int timestamp, ImageArray frame);
    void saveIntSeries(const IntArray array, const QByteArray name);
    void saveDoubleSeries(const DoubleArray array, const QByteArray name);
    void start();
    void stop();

  private:
    // HDF5 related
    hid_t hFile;
    hid_t hFrameData, hFrameTimestamp;
    hid_t hImageType;

    bool is_initialized, needs_cleanUp;
    RecordParams *params;
    RegionOfInterest* roi;

    void finishSaving();
};

#endif // SAVEWORKER_H
