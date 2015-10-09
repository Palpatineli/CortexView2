#ifndef CAMERA_H
#define CAMERA_H

#include <QObject>
#include <QRect>
#include <QVector>
#include <QPair>

#ifndef _MASTER_H
typedef unsigned short uns16;
#endif  // _MASTER_H
#ifndef _PVCAM_H
/************************* Class 3: Region Definition ************************/
typedef struct {
    uns16 s1;                     /* First pixel in the serial register */
    uns16 s2;                     /* Last pixel in the serial register */
    uns16 sbin;                   /* Serial binning for this region */
    uns16 p1;                     /* First row in the parallel register */
    uns16 p2;                     /* Last row in the parallel register */
    uns16 pbin;                   /* Parallel binning for this region */
} rgn_type;
#endif  // _PVCAM_H

typedef QVector<quint16> ImageArray;
class RecordParams;

class Camera : public QObject {
    Q_OBJECT
  public:
    explicit Camera(QObject *parent = 0);
    ~Camera();

    static void rect2rgn(const QRect &source, rgn_type &dest);
    static void rgn2rect(const rgn_type &source, QRect &dest);

    QRect getROI() const;
    bool isReady() const;

  signals:
    void yieldFrame(const quint64 timestamp, const double phase, const ImageArray image_ptr, const QRect roi_ptr);
    void raiseError(QString error_message);
    void yieldTemperature(int temperature);

  public slots:
    void setROI(const QRect& roi_in = QRect(0, 0, 512, 512));
    void init();
    void checkTemp();

  private:
    const quint32 CIRCULAR_BUFFER_FRAME_NO = 20;
    int timer_id;
    quint64 debug_counter;

    // running parameters
    RecordParams* params;
    bool needs_clean_up, is_running, is_initialized, callback_registered;
    //
    QByteArray camera_name;
    qint16 hCam;
    QRect ccd_size, roi;
    quint64 frame_size;
    quint16* circ_buffer;

    void onError(QString function_name);
    void lookUpError(QString function_name);
    void cleanup();

    void startAcquisition();
    void stopAcquisition();

    void captureFrame();
    bool assertParamAvailability(quint32 param_id, QString param_name);
    bool setParam(quint32 param_id, QString param_name, void* param_value);
    bool initCCDSize();
    bool initSpeedTable(QVector<QPoint> &indices, QVector<float> &readout_freq, QVector<quint16> &bit_depth);
    QVector<QPair<qint32, QByteArray>> enumerateParameter(quint32 param_id, QString param_name);

    friend void camCallback(void* contetn);
};

void camCallback(void* contetn);

#endif // CAMERA_H
