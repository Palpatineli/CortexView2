#include <QPainter>
#include <QMouseEvent>
#include <QDebug>
#include "regionofinterest.h"
#include "cameraview.h"

CameraView::CameraView(QWidget *parent) : QWidget(parent),
    is_drawing_roi(false), is_needing_redraw(false), is_waiting_incoming(false),
    status("Standby"), status_color(Qt::green) {
    roi = RegionOfInterest::getRegion();
    draw_roi = roi->getROI();
    timer.start(13, Qt::CoarseTimer, this);
}

CameraView::~CameraView() {
}

void CameraView::paintEvent(QPaintEvent * /*event*/) {
    QPainter painter(this);
    if (image.isNull()) {
        painter.fillRect(rect(), Qt::gray);
    } else if (is_needing_redraw) {
        painter.drawImage(roi->getROI(), image);
        painter.setBrush(Qt::NoBrush);
        painter.setPen(Qt::green);
        painter.drawRect(draw_roi);
        paintStatus();
        is_needing_redraw = false;
    } else {
        painter.setClipRect(roi->getROI());
        painter.drawImage(roi->getROI(), image);
        paintStatus();
    }
}

void CameraView::paintStatus() {
    QPainter painter(this);
    QFontMetrics metrics = painter.fontMetrics();
    QString temp;
    if (status == "REC") {
        temp = status + " " + time_str;
    } else {
        temp = status;
    }

    int text_width = metrics.width(temp);

    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::gray);
    painter.drawRect(5, 5, text_width + 10, metrics.lineSpacing() + 5);
    painter.setPen(status_color);
    painter.drawText(10, metrics.leading() + metrics.ascent() + 5, temp);
}

void CameraView::timerEvent(QTimerEvent * /*event*/) {
    update();
}

void CameraView::updateStatus(QString status_in, Qt::GlobalColor status_color_in) {
    status = status_in;
    status_color = status_color_in;
    is_needing_redraw = true;
}

void CameraView::mousePressEvent(QMouseEvent *event) {
    if (roi->isLocked()) return;
    dragStart = event->pos();
    is_drawing_roi = true;
}

void CameraView::mouseMoveEvent(QMouseEvent *event) {
    if (is_drawing_roi) setROI(QRect(dragStart, event->pos()).normalized());
}

void CameraView::mouseReleaseEvent(QMouseEvent *event) {
    if (is_drawing_roi && !roi->isLocked()) {
        is_drawing_roi = false;
        if (event->pos() == dragStart) return;
        roi->setROI(dragStart, event->pos());
        emit(roiChanged());
    }
}

void CameraView::setROI(QRect new_roi) {
    draw_roi = new_roi;
    is_needing_redraw = true;
    is_waiting_incoming = true;
}

void CameraView::updateImage(const QImage image_in) {
    image = image_in;
    if (is_waiting_incoming) {
        is_needing_redraw = true;
        is_waiting_incoming = false;
    } else draw_roi = roi->getROI();
}

void CameraView::resetROI() {
    if (roi->isLocked()) return;
    roi->reset();
    emit(roiChanged());
}

bool CameraView::toggleBin(bool is2by2) {
    /* toggles binning
     * returns the current state, true for 2x2 bin and false for 1x1 no bin
     */
    if (roi->isLocked()) return !is2by2;
    if (is2by2) {
        roi->setBins(2, 2);
    } else {
        roi->setBins(1, 1);
    }
    emit(roiChanged());
    return is2by2;

}

void CameraView::updateRemainingTime(int seconds) {
    int minutes = int(seconds / 60);
    seconds -= minutes * 60;
    time_str = QString("%1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0'));
}
