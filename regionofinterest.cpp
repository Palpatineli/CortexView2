#include <QReadLocker>
#include <QWriteLocker>
#include "regionofinterest.h"

RegionOfInterest::RegionOfInterest()
    : roi(0, 0, 512, 512), roi_limit(0, 0, 512, 512), x_bin(2), y_bin(2), is_locked(false) {
}

QRect RegionOfInterest::getROI() {
    QReadLocker locker(&roi_lock);
    return roi;
}

rgn_type RegionOfInterest::getRGN() {
    rgn_type new_rgn;
    QReadLocker locker(&roi_lock);
    new_rgn.s1 = roi.left();
    new_rgn.s2 = roi.right();
    new_rgn.sbin = x_bin;
    new_rgn.p1 = roi.top();
    new_rgn.p2 = roi.bottom();
    new_rgn.pbin = y_bin;
    return new_rgn;
}

void RegionOfInterest::setROI(const QRect &value) {
    if (is_locked) return;
    QWriteLocker locker(&roi_lock);
    roi = value.normalized();
    validateROI();
}

void RegionOfInterest::setROI(const int x, const int y, const int width, const int height) {
    if (is_locked) return;
    QWriteLocker locker(&roi_lock);
    roi.setCoords(x, y, x+width-1, y+height-1);
    roi = roi.normalized();
    validateROI();
}

void RegionOfInterest::setROI(const rgn_type &rgn_in) {
    if (is_locked) return;
    QWriteLocker locker(&roi_lock);
    roi.setCoords(rgn_in.s1, rgn_in.p1, rgn_in.s2, rgn_in.p2);
    x_bin = rgn_in.sbin;
    y_bin = rgn_in.pbin;
    validateROI();
}

void RegionOfInterest::setROI(const QPoint &top_left, const QPoint &bottom_right) {
    if (is_locked) return;
    QWriteLocker locker(&roi_lock);
    roi.setTopLeft(top_left);
    roi.setBottomRight(bottom_right);
    roi = roi.normalized();
    validateROI();
}

void RegionOfInterest::setROILimit(const QRect &value) {
    if (is_locked) return;
    QWriteLocker locker(&roi_lock);
    roi_limit = value.normalized();
    validateROI();
}

void RegionOfInterest::setROILimit(const int x, const int y, const int width, const int height) {
    if (is_locked) return;
    QWriteLocker locker(&roi_lock);
    roi_limit.setCoords(x, y, x+width-1, y+height-1);
    roi_limit = roi_limit.normalized();
    validateROI();
}

QPoint RegionOfInterest::getBins(){
    QReadLocker locker(&roi_lock);
    return QPoint(x_bin, y_bin);
}

void RegionOfInterest::setBins(const int x_in, const int y_in){
    if (is_locked) return;
    QWriteLocker locker(&roi_lock);
    x_bin = std::abs(x_in);
    y_bin = std::abs(y_in);
    validateROI();
}

void RegionOfInterest::setBins(const QPoint &bins_in) {
    if (is_locked) return;
    QWriteLocker locker(&roi_lock);
    x_bin = std::abs(bins_in.x());
    y_bin = std::abs(bins_in.y());
    validateROI();
}


RegionOfInterest *RegionOfInterest::getRegion() {
    static RegionOfInterest singleton;
    return &singleton;
}

int RegionOfInterest::length() {
    QReadLocker locker(&roi_lock);
    return (roi.width() * roi.height()) / (x_bin * y_bin);
}

QSize RegionOfInterest::getSize() {
    return QSize(roi.width() / x_bin, roi.height() / y_bin);
}

void RegionOfInterest::reset() {
    QWriteLocker locker(&roi_lock);
    roi = roi_limit;
    validateROI();
}

bool RegionOfInterest::validateROI() {
    if (!roi_limit.contains(roi)) roi &= roi_limit;
    if (roi.width() % x_bin != 0) {
        int width = (int(roi.width() / x_bin) + 1) * x_bin;
        if (width + roi.x() - 1 > roi_limit.right()) {
            width -= x_bin;
            if (width <= 0) return false;
        }
        roi.setWidth(width);
    }
    if (roi.height() % x_bin != 0) {
        int height = (int(roi.height() / y_bin) + 1) * y_bin;
        if (height + roi.y() - 1 > roi_limit.bottom()) {
            height -= y_bin;
            if (height <= 0) return false;
        }
        roi.setHeight(height);
    }
    return true;
}

bool RegionOfInterest::lock() {
    QWriteLocker lock_locker(&lock_lock);
    if (is_locked) return false;
    is_locked = true;
    return true;
}

bool RegionOfInterest::isLocked() {
    QReadLocker lock_locker(&lock_lock);
    return is_locked;
}

void RegionOfInterest::unlock() {
    QWriteLocker lock_locker(&lock_lock);
    is_locked = false;
}
