/* wraps HDF5 c library to save images, diode signal train, and related metadata.
 * no error handling. It's not likely to have error.
 */
#include <QRect>
#include <QDebug>

#include "hdf5.h"
#include "hdf5_hl.h"
#include "recordparams.h"
#include "saveworker.h"

SaveWorker::SaveWorker(QRect roi_in, QObject *parent) : QObject(parent),
    is_initialized(false), needs_cleanUp(false), roi(roi_in), frame_no(0) {
    params = RecordParams::getParams();
}

void SaveWorker::start() {
    QByteArray file_name = params->getFile_path().toLocal8Bit();
    hFile = H5Fcreate(file_name.constData(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    needs_cleanUp = true;
    int period_in_frames = params->getPeriod_in_frames();
    H5LTset_attribute_int(hFile, "/", "period_in_frames", &period_in_frames, 1);
    int period_in_seconds = params->getPeriod_in_seconds();
    H5LTset_attribute_int(hFile, "/", "period_in_seconds", &period_in_seconds, 1);
    int cycle_no = params->getCycle_no();
    H5LTset_attribute_int(hFile, "/", "cycle_no", &cycle_no, 1);

    // create custom hdf5 data types
    hsize_t image_dims[2] = {roi.width(), roi.height()};
    H5T_IMAGE = H5Tarray_create(H5T_NATIVE_UINT16, 2, image_dims);

    hFrames = H5Gcreate(hFile, "frames", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    hPTable = H5PTcreate_fl(hFrames, "frame_table", H5T_IMAGE, 32, -1);
    int roi_attr[4] = {roi.left(), roi.top(), roi.right(), roi.bottom()};
    H5LTset_attribute_int(hFile, "frames", "roi", roi_attr, 4);

    hFrameTimestamp = H5PTcreate_fl(hFrames, "frame_timestamps", H5T_NATIVE_UINT64, 128, 5);
    hFramePhase = H5PTcreate_fl(hFrames, "frame_phases", H5T_NATIVE_DOUBLE, 128, 5);
    is_initialized = true;
}

void SaveWorker::pushDiodeSignal(quint64 timestamp, double signal_size) {
    if (!is_initialized) return;
    return;
    diode_signal<<signal_size;
    diode_timestamp<<timestamp;
}

void SaveWorker::pushFrame(const quint64 timestamp, const double frame_phase, ImageArray frame, QRect roi_in) {
    if (!is_initialized) return;
    if (!(roi_in == roi)) {
        emit(raiseError("roi != roi_in"));
        frame_no++;
        if (frame_no > 1000) stop();
        return;
    }
    H5PTappend(hPTable, 1, frame.constData());
    H5PTappend(hFrameTimestamp, 1, &timestamp);
    H5PTappend(hFramePhase, 1, &frame_phase);
    H5Fflush(hFile, H5F_SCOPE_GLOBAL);
    frame_no++;
    if (frame_no > 1000) stop();
}

void SaveWorker::cleanUp() {
    is_initialized = false;
    H5Fflush(hFile, H5F_SCOPE_GLOBAL);
    H5PTclose(hFramePhase);
    H5PTclose(hFrameTimestamp);
    H5PTclose(hPTable);
    H5Fclose(hFile);
    qDebug() << "after cleanup";
    emit(finished());
}

void SaveWorker::stop() {
    is_initialized = false;
    hDiodes = H5Gcreate(hFile, "diodes", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    foreach(double i, diode_signal) {
        qDebug() << i;
    }

    hsize_t dims[1] = {diode_signal.length()};
    hid_t timestamp_space = H5Screate_simple(1, dims, NULL);
    hDiodeSignal = H5LTmake_dataset_double(hDiodes, "diode_signal", 1, dims, diode_signal.constData());
    hDiodeTimestamp = H5Dcreate(hDiodes, "diode_timestamps", H5T_NATIVE_UINT64, timestamp_space, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    H5Dwrite(hDiodeTimestamp, H5T_NATIVE_UINT64, H5S_ALL, H5S_ALL, H5P_DEFAULT, diode_timestamp.data());
    H5Fflush(hFile, H5F_SCOPE_GLOBAL);
    qDebug() << "before cleanup";
    cleanUp();
}
