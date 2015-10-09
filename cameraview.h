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

    void updateStatus(const QString status, const Qt::GlobalColor status_color);

signals:
    void roiChangeEvent(const QRect new_roi);

public slots:
    void updateImage(const QImage image_in, const QRect draw_roi);
    void setROI(const QRect new_roi = QRect(0, 0, 512, 512));

protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void timerEvent(QTimerEvent *event) Q_DECL_OVERRIDE;  // update on timer

private:
    const QColor TRANSPARENT_BLACK = QColor(0, 0, 0, 127);

    QBasicTimer timer;
    QReadWriteLock* lock;
    QImage image;

    QPoint dragStart;
    QRect draw_roi, roi;
    bool is_drawing_roi, is_needing_redraw, is_waiting_incoming;
    QVector<QRect> roi_inverse;

    QString status;
    Qt::GlobalColor status_color;

    void paintStatus();
};

#endif // CAMERAVIEW_H
