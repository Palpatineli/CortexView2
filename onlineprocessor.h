#ifndef ONLINEPROCESSOR_H
#define ONLINEPROCESSOR_H

#include <QObject>
#include <QRect>
#include <QVector>
#include <QImage>

typedef QVector<quint16> ImageArray;
typedef QVector<qreal> DFTArray;

class OnlineProcessor : public QObject
{
    Q_OBJECT
public:
    explicit OnlineProcessor(QObject *parent = 0);

signals:
    void yieldImage(const QImage image, const QRect roi);
    void raiseError(QString error_message);

public slots:
    void updateImage(const quint64 timestamp, const double phase, const ImageArray image, const QRect roi);
    void startRecording();
    void stopRecording();
    void takePicture(QString file_path);

private:
    QRect roi;
    DFTArray real;
    DFTArray imag;
    QString file_path;
    bool is_recording, take_next_picture;
    quint64 frame_no;

    QImage drawBufferFromRaw(const ImageArray image);
    void processFrame(const ImageArray image, qreal phase);
    QImage drawBufferFromDFT();
};

#endif // ONLINEPROCESSOR_H
