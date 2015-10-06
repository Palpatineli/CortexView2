#ifndef CAMERA_H
#define CAMERA_H

#include <QObject>
#include <QRect>
#include <QVector>

#ifndef _MASTER_H
typedef unsigned short uns16, * uns16_ptr;
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

class Camera : public QObject {
    Q_OBJECT
  public:
    explicit Camera(QObject *parent = 0);
    ~Camera();

    static void rect2rgn(const QRect &source, rgn_type &dest);
    static void rgn2rect(const rgn_type &source, QRect &dest);

  signals:
    void yieldFrame(const quint64 timestamp, const double phase, const ImageArray image_ptr, const QRect roi_ptr);
    void raiseError(QString error_message);
    void cameraReady();
    void cameraStarted();
    void cameraStopped();

  public slots:
    void captureFrame(const quint64 timestamp, const double phase);
    void setROI(const QRect &roi_in);
    void init();  // returned opend camera name
    void cleanUp(QString function_name);
    void setRecording(bool is_recording);

  private:
    const quint32 CIRCULAR_BUFFER_FRAME_NO = 20;

    // running parameters
    bool needs_clean_up, is_recording, is_running, is_initialized;
    //
    QByteArray camera_name;
    qint16 hCam;
    QRect ccd_size;
    rgn_type roi;
    quint64 buffer_size;
    uns32 stream_size;
    QVector<quint16> circ_buffer;

    void startAcquisition();
    void stopAcquisition();
    void lookUpError(QString function_name);
    bool assertParamAvailability(quint32 param_id);
    bool initCCDSize();
    bool initSpeedTable(QVector<QPoint> &indices, QVector<float> &readout_freq, QVector<quint16> &bit_depth);

};

#endif // CAMERA_H
