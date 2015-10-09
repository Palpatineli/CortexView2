#include <QPainter>
#include <QMouseEvent>
#include <QDebug>
#include "cameraview.h"

CameraView::CameraView(QWidget *parent) : QWidget(parent),
    draw_roi(0, 0, 512, 512),
    is_drawing_roi(false), is_needing_redraw(false), is_waiting_incoming(false),
    status("Standby"), status_color(Qt::green) {
    roi_inverse << QRect(0, 0, 512, 0) << QRect(512, 0, 0, 512) << QRect(0, 512, 512, 0) << QRect(0, 0, 0, 512);
    timer.start(13, Qt::CoarseTimer, this);
}

CameraView::~CameraView() {
}

void CameraView::paintEvent(QPaintEvent * /*event*/) {
    QPainter painter(this);
    if (image.isNull()) {
        painter.fillRect(rect(), Qt::gray);
    } else if (is_needing_redraw) {
        painter.drawImage(roi, image);
        painter.setPen(Qt::NoPen);
        painter.setBrush(TRANSPARENT_BLACK);
        painter.drawRects(roi_inverse);
        paintStatus();
        is_needing_redraw = false;
    } else {
        painter.setClipRect(draw_roi);
        painter.drawImage(roi, image);
    }
}

void CameraView::paintStatus() {
    QPainter painter(this);
    QFontMetrics metrics = painter.fontMetrics();
    int text_width = metrics.width(status);

    painter.setPen(Qt::NoPen);
    painter.setBrush(TRANSPARENT_BLACK);
    painter.drawRect(5, 5, text_width + 10, metrics.lineSpacing() + 5);
    painter.setPen(Qt::white);
    painter.drawText(10, metrics.leading() + metrics.ascent() + 5, status);
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
    if (!is_drawing_roi) {
        dragStart = event->pos();
        is_drawing_roi = true;
    }
}

void CameraView::mouseMoveEvent(QMouseEvent *event) {
    if (is_drawing_roi) {
        setROI(QRect(dragStart, event->pos()).normalized());
    }
}

void CameraView::mouseReleaseEvent(QMouseEvent *event) {
    if (is_drawing_roi) {
        is_drawing_roi = false;
        setROI(QRect(dragStart, event->pos()).normalized());
        emit(roiChangeEvent(draw_roi));
    }
}

void CameraView::setROI(QRect new_roi) {
    draw_roi = new_roi;
    roi_inverse[0].moveBottom(draw_roi.top());
    roi_inverse[1].moveLeft(draw_roi.right());
    roi_inverse[2].moveTop(draw_roi.bottom());
    roi_inverse[3].moveRight(draw_roi.left());
    is_needing_redraw = true;
    is_waiting_incoming = true;
}

void CameraView::updateImage(const QImage image_in, const QRect update_rect) {
    image = image_in;
    roi = update_rect;
    if (is_waiting_incoming) {
        is_needing_redraw = true;
        is_waiting_incoming = false;
    }
}
