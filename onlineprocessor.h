#ifndef ONLINEPROCESSOR_H
#define ONLINEPROCESSOR_H

#include <QObject>
#include <QRect>
#include <QVector>
#include <QImage>
#include <QPair>

typedef QVector<quint16> ImageArray;
typedef QVector<double> DoubleArray;
typedef QVector<int> IntArray;
class RecordParams;
class RegionOfInterest;

class OnlineProcessor : public QObject {
    Q_OBJECT
  public:
    explicit OnlineProcessor(QObject *parent = 0);
    static void complex2rgb(double real_element, double imag_element, uchar* argb_ptr);

  signals:
    void yieldImage(const QImage image);
    void yieldImageData(const int timestamp, const ImageArray image);
    void saveIntSeries(const IntArray array, QByteArray name);
    void saveDoubleSeries(const DoubleArray array, QByteArray name);
    void raiseError(QString error_message);
    void reportRemainingTime(int seconds);
    void finishSaving();

  public slots:
    void pushImage(const int timestamp, const ImageArray image);
    void pushDiodeSignal(const int timestamp, const double signal);
    void pushCameraSignal(const int timestamp, const int computer_time);
    void startRecording();
    void stopRecording();
    void takePicture(QString file_path);

  private:
    const quint32 frames_per_pulse = 9;

    int slow_dft_counter;

    RegionOfInterest* roi;
    DoubleArray real;
    DoubleArray imag;
    QString file_path;
    bool is_recording, take_next_picture, stimulus_started;
    RecordParams *params;
    int start_time, end_time;

    // phase calculation for online analysis
    double phase_per_pulse, last_diode_phase, phase_per_ms, phase_exposure_offset;
    int pulse_per_cycle, pulse_id;

    // online analysis of frame phase
    IntArray diode_timestamp;
    DoubleArray diode_signal;
    IntArray camera_expose_timestamp;
    IntArray camera_expose_computer_time;

    QImage drawBufferFromRaw(const ImageArray image);
    void processFrame(const ImageArray image, const double phase);
    QImage drawBufferFromDFT();

};

#endif // ONLINEPROCESSOR_H
