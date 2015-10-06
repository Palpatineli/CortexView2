#ifndef CAMERAVIEW_H
#define CAMERAVIEW_H

#include <QPixmap>
#include <QWidget>
#include <QVector>
#include <QBasicTimer>

class QReadWriteLock;

class CameraView : public QWidget
{
    Q_OBJECT
public:
    explicit CameraView(QWidget *parent = 0);
    ~CameraView();

signals:
    void roiChangeEvent(QRect new_roi);

public slots:
    void updateImage(const uchar* const image_in, const QRect& update_rect);
    void updateStatus(QString status, Qt::GlobalColor status_color);
    void updateRoi(QRect new_roi);

protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void timerEvent(QTimerEvent *event) Q_DECL_OVERRIDE;  // update on timer

private:
    const QColor TRANSPARENT_BLACK = QColor(0, 0, 0, 127);

    QBasicTimer timer;
    QImage image;
    QReadWriteLock* lock;

    QPoint dragStart;
    bool is_drawing_roi, is_needing_redraw, is_waiting_incoming;
    QRect roi;
    QVector<QRect> roi_inverse;

    QString status;
    Qt::GlobalColor status_color;

    void paintStatus();
    void paintRoi();
    void histogram();
};

#endif // CAMERAVIEW_H
