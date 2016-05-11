#include <QDir>
#include <QStandardPaths>
#include <QMutexLocker>
#include "recordparams.h"

RecordParams::RecordParams()
    : period_in_frames(900), period_in_seconds(12), cycle_no(20), is_locked(false), exposure_time(30),
    frame_per_pulse(6){
    QDir file_folder(QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
    file_path = file_folder.filePath("Data.h5");
}
int RecordParams::getFramePerPulse() const
{
    return frame_per_pulse;
}

void RecordParams::setFramePerPulse(int value)
{
    frame_per_pulse = value;
}

int RecordParams::getExposureTime() const {
    return exposure_time;
}

void RecordParams::setExposureTime(int value) {
    exposure_time = value;
}


int RecordParams::getCycleNo() const {
    return cycle_no;
}

void RecordParams::setCycleNo(const int value) {
    if (is_locked) return;
    cycle_no = value;
}

double RecordParams::getPeriodInSeconds() const {
    return period_in_seconds;
}

void RecordParams::setPeriodInSeconds(const double value) {
    if (is_locked) return;
    period_in_seconds = value;
}

int RecordParams::getPeriodInFrames() const {
    return period_in_frames;
}

void RecordParams::setPeriodInFrames(const int value) {
    if (is_locked) return;
    period_in_frames = value;
}

QString RecordParams::getFilePath() {
    QMutexLocker locker(&file_path_lock);
    return file_path;
}

void RecordParams::setFilePath(const QString &value) {
    if (is_locked) return;
    QMutexLocker locker(&file_path_lock);
    file_path = value;
}

bool RecordParams::lock() {
    QMutexLocker lock_locker(&lock_lock);
    if (is_locked) return false;
    is_locked = true;
    return true;
}

void RecordParams::unlock() {
    is_locked = false;
}

bool RecordParams::isLocked() const {
    return is_locked;
}

RecordParams *RecordParams::getParams() {
    static RecordParams singleton;
    return &singleton;
}
