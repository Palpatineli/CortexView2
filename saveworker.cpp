/* wraps HDF5 c library to save images, diode signal train, and related metadata.
 * no error handling. It's not likely to have error.
 */
#include <QRect>
#include <QDebug>
#include <QImage>
#include <QImageWriter>

#include <hdf5.h>
#include <hdf5_hl.h>
#include "recordparams.h"
#include "regionofinterest.h"
#include "saveworker.h"

SaveWorker::SaveWorker(QObject *parent) : QObject(parent),
    is_initialized(false), needs_cleanUp(false) {
    params = RecordParams::getParams();
    roi = RegionOfInterest::getRegion();
}

void SaveWorker::start() {
    params->lock();
    QByteArray file_name = params->getFilePath().toLocal8Bit();
    hFile = H5Fcreate(file_name.constData(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    needs_cleanUp = true;
    int period_in_frames = params->getPeriodInFrames();
    H5LTset_attribute_int(hFile, "/", "period_in_frames", &period_in_frames, 1);
    int period_in_seconds = params->getPeriodInSeconds();
    H5LTset_attribute_int(hFile, "/", "period_in_seconds", &period_in_seconds, 1);
    int cycle_no = params->getCycleNo();
    H5LTset_attribute_int(hFile, "/", "cycle_no", &cycle_no, 1);
    int exposure_time = params->getExposureTime();
    H5LTset_attribute_int(hFile, "/", "exposure_time", &exposure_time, 1);

    QRect rect = roi->getROI();
    QPoint bins = roi->getBins();
    int roi_attr[6] = {rect.left(), rect.top(), rect.right(), rect.bottom(), bins.x(), bins.y()};
    H5LTset_attribute_int(hFile, "/", "roi", roi_attr, 6);

    // create custom hdf5 data types
    QSize size = roi->getSize();
    hsize_t image_dims[2] = {hsize_t(size.height()), hsize_t(size.width())};
    hImageType = H5Tarray_create(H5T_NATIVE_UINT16, 2, image_dims);

    hFrameData = H5PTcreate_fl(hFile, "frame_data", hImageType, 32, -1);

    hFrameTimestamp = H5PTcreate_fl(hFile, "frame_timestamps", H5T_NATIVE_INT, 128, 5);

    is_initialized = true;
    emit(started());
}

void SaveWorker::pushFrame(const int timestamp, ImageArray frame) {
    if (!is_initialized) return;
    if (frame.length() != roi->length()) {
        emit(raiseError("roi != roi_in"));
        return;
    }
    H5PTappend(hFrameData, 1, frame.constData());
    H5PTappend(hFrameTimestamp, 1, &timestamp);
    H5Fflush(hFile, H5F_SCOPE_GLOBAL);
}

void SaveWorker::stop() {
    is_initialized = false;
    H5Fflush(hFile, H5F_SCOPE_GLOBAL);
    H5PTclose(hFrameTimestamp);
    H5PTclose(hFrameData);
    H5Fclose(hFile);
    qDebug() << "after saveworker cleanup";
    params->unlock();
    roi->unlock();
    emit(finished());
}

void SaveWorker::saveDoubleSeries(const DoubleArray array, const QByteArray name) {
    hsize_t array_dims[1] = {hsize_t(array.length())};
    H5LTmake_dataset_double(hFile, name.constData(), 1, array_dims, array.constData());
    qDebug() << "saved " << name;
}

void SaveWorker::saveIntSeries(const IntArray array, const QByteArray name) {
    hsize_t array_dims[1] = {hsize_t(array.length())};
    H5LTmake_dataset_int(hFile, name.constData(), 1, array_dims, array.constData());
    qDebug() << "saved " << name;
}
