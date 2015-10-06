#include <QPainter>
#include <QMouseEvent>
#include "cameraview.h"

CameraView::CameraView(QWidget *parent) :
    QWidget(parent),
    status("Loading"), status_color(Qt::yellow), roi(0, 0, 512, 512),
    is_drawing_roi(false), is_needing_redraw(false),
    is_waiting_incoming(false),
    image(512, 512, QImage::Format_ARGB32){
    timer.start(13, this);
    roi_inverse<<QRect(0, 0, 512, 0)<<QRect(512, 0, 0, 512)<<QRect(0, 512, 512, 0)<<QRect(0, 0, 0, 512);
}

CameraView::~CameraView() {
}

void CameraView::paintEvent(QPaintEvent */*event*/) {
    QPainter painter(this);
    if (image.isNull()) {
        painter.fillRect(rect(), Qt::gray);
    } else if (is_needing_redraw){
        painter.drawImage(rect(), image, rect());
        painter.setPen(Qt::NoPen);
        painter.setBrush(TRANSPARENT_BLACK);
        painter.drawRects(roi_inverse);
        paintStatus();
        is_needing_redraw = false;
    } else {
        painter.setClipRect(roi);
        painter.drawImage(roi, image, roi);
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

void CameraView::histogram() {
    QVector<quint32> hist(100);
}

void CameraView::timerEvent(QTimerEvent */*event*/) {
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
        updateRoi(QRect(dragStart, event->pos()).normalized());
    }
}

void CameraView::mouseReleaseEvent(QMouseEvent *event) {
    if (is_drawing_roi) {
        emit(roiChangeEvent(roi));
        is_drawing_roi = false;
        updateRoi(QRect(dragStart, event->pos()).normalized());
    }
}

void CameraView::updateRoi(QRect new_roi) {
    roi = new_roi;
    roi_inverse[0].moveBottom(roi.top());
    roi_inverse[1].moveLeft(roi.right());
    roi_inverse[2].moveTop(roi.bottom());
    roi_inverse[3].moveRight(roi.left());
    is_needing_redraw = true;
    is_waiting_incoming = true;
}

void CameraView::updateImage(const uchar * const image_in, const QRect &update_rect) {
    int bottom = update_rect.bottom();
    int right = update_rect.right() * 4;
    int left = update_rect.left() * 4;
    for (int i = update_rect.top(); i < bottom; i++) {
        uchar *image_line = image.scanLine(i);
        int offset = i * update_rect.width() * 4;
        for (int j = left; j < right; j++) {
            image_line[j] = image_in[offset + j];
        }
    }
    if (is_waiting_incoming) {
        is_needing_redraw = true;
        is_waiting_incoming = false;
    }
}
