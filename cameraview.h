#ifndef CAMERAVIEW_H
#define CAMERAVIEW_H

#include <QImage>
#include <QWidget>
#include <QBasicTimer>

class QReadWriteLock;
class RegionOfInterest;

class CameraView : public QWidget {
    Q_OBJECT
  public:
    explicit CameraView(QWidget *parent = 0);
    ~CameraView();

    void updateStatus(const QString status, const Qt::GlobalColor status_color);
    void resetROI();
    bool toggleBin(bool is2by2);

  signals:
    void roiChanged();

  public slots:
    void updateImage(const QImage image_in);
    void updateRemainingTime(const int seconds);

  protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void timerEvent(QTimerEvent *event) Q_DECL_OVERRIDE;  // update on timer

  private:
    QBasicTimer timer;
    QReadWriteLock *lock;
    QImage image;

    QPoint dragStart;
    RegionOfInterest* roi;
    QRect draw_roi;
    bool is_drawing_roi, is_needing_redraw, is_waiting_incoming;

    QString status;
    QString time_str;
    Qt::GlobalColor status_color;

    void paintStatus();
    void setROI(QRect new_roi);
};

#endif // CAMERAVIEW_H
