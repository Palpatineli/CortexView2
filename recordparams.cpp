#include <QDir>
#include <QStandardPaths>
#include <QReadLocker>
#include <QWriteLocker>
#include "recordparams.h"

RecordParams::RecordParams()
    : period_in_frames(900), period_in_seconds(12), cycle_no(20), is_recording(false) {
    QDir file_folder(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    file_path = file_folder.filePath("Data.h5");
}

int RecordParams::getCycle_no() {
    QReadLocker lock(&cycle_no_lock);
    return cycle_no;
}

void RecordParams::setCycle_no(int value) {
    {
        QReadLocker lock_lock(&recording_lock);
        if (is_recording) return;
    }
    QWriteLocker lock(&cycle_no_lock);
    cycle_no = value;
}

double RecordParams::getPeriod_in_seconds() {
    QReadLocker lock(&period_in_seconds_lock);
    return period_in_seconds;
}

void RecordParams::setPeriod_in_seconds(double value) {
    {
        QReadLocker lock_lock(&recording_lock);
        if (is_recording) return;
    }
    QWriteLocker lock(&period_in_seconds_lock);
    period_in_seconds = value;
}

int RecordParams::getPeriod_in_frames() {
    QReadLocker lock(&period_in_frames_lock);
    return period_in_frames;
}

void RecordParams::setPeriod_in_frames(int value) {
    {
        QReadLocker lock_lock(&recording_lock);
        if (is_recording) return;
    }
    QWriteLocker lock(&period_in_frames_lock);
    period_in_frames = value;
}

QString RecordParams::getFile_path() {
    QReadLocker lock(&file_path_lock);
    return file_path;
}

void RecordParams::setFile_path(const QString &value) {
    {
        QReadLocker lock_lock(&recording_lock);
        if (is_recording) return;
    }
    QWriteLocker lock(&file_path_lock);
    file_path = value;
}

bool RecordParams::lock() {
    QWriteLocker lock_lock(&recording_lock);
    if (is_recording) return false;
    is_recording = true;
    return true;
}

void RecordParams::unlock() {
    QWriteLocker lock_lock(&recording_lock);
    is_recording = false;
}

RecordParams *RecordParams::getParams() {
    static RecordParams singleton;
    return &singleton;
}
