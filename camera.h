#ifndef CAMERA_H
#define CAMERA_H

#include <QObject>
#include <QRect>
#include <QVector>
#include <QPair>

typedef QVector<quint16> ImageArray;
class RecordParams;
class RegionOfInterest;

class Camera : public QObject {
    Q_OBJECT
  public:
    explicit Camera(QObject *parent = 0);
    ~Camera();

    bool isReady() const;

  signals:
    void yieldFrame(const int computer_time, const ImageArray image);
    void yieldTemperature(int temperature);
    void raiseError(QString error_message);

  public slots:
    void setROI();
    void init();
    void checkTemp();

  private:
    const quint32 CIRCULAR_BUFFER_FRAME_NO = 40;
    int timer_id;
    quint64 debug_counter;

    // running parameters
    RecordParams *params;
    RegionOfInterest* roi;
    bool needs_clean_up, is_running, is_initialized, callback_registered;
    //
    QByteArray camera_name;
    qint16 hCam;
    quint64 frame_size;
    ImageArray buffer;

    void onError(QString function_name);
    void lookUpError(QString function_name);
    void cleanup();

    void startAcquisition();
    void stopAcquisition();

    void captureFrame();

    bool assertParamAvailability(quint32 param_id, QString param_name);
    bool setParam(quint32 param_id, QString param_name, void *param_value);
    bool initCCDSize();
    bool initSpeedTable(QVector<QPoint> &indices, QVector<float> &readout_freq, QVector<quint16> &bit_depth);
    bool initCameraInfo();
    QVector<QPair<qint32, QByteArray>> enumerateParameter(quint32 param_id, QString param_name);

    friend void camCallback(void *content);
};

void camCallback(void *content);

#endif // CAMERA_H
