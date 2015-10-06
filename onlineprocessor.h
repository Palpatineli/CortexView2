#ifndef ONLINEPROCESSOR_H
#define ONLINEPROCESSOR_H

#include <QObject>
#include <QRect>

class OnlineProcessor : public QObject
{
    Q_OBJECT
public:
    explicit OnlineProcessor(QObject *parent = 0);

signals:
    void yieldImage(const uchar* image[512][2048], const QRect& roi);
    void yieldDFT(const qreal* result_real[512][512], const qreal* result_imag[512][512]);

public slots:
    void updateImage(quint64 timestamp, double phase, const QSharedPointer<QVector<quint16>> image_ptr, QSharedPointer<QRect> roi_ptr);
    void startRecording(const QRect& roi);
    void stopRecording();
    void takePicture();

private:
    quint16* buffer[512][512];
    uchar draw_buffer[512][2048];
    QRect roi;
    qreal onlineAddition_real[512][512];
    qreal onlineAddition_imag[512][512];
    bool is_recording;
    quint64 frame_no;
    qreal phase;

    void drawBufferFromRaw();
    void drawBufferFromRawHighlight();
    void processFrame();
};

#endif // ONLINEPROCESSOR_H
