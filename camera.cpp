#include <master.h>
#include <pvcam.h>
#include <QDebug>
#include <QTime>
#include "recordparams.h"

#include "camera.h"

Camera::Camera(QObject *parent) : QObject(parent),
    is_initialized(false), needs_clean_up(false), ccd_size(0, 0, 512, 512), roi(0, 0, 512, 512), is_running(false), debug_counter(0) {
    params = RecordParams::getParams();
}

Camera::~Camera() {
    cleanup();
}

void Camera::init() {
    if (is_initialized) {emit(raiseError("Can't repeatedly init camera")); return;}
// initialize PVcam
    if (PV_FAIL == pl_pvcam_init()) {onError("pl_pvcam_init()"); return;}
    needs_clean_up = true;
    if (PV_FAIL == pl_exp_init_seq()) {onError("pl_exp_init_seq()"); return;}
    qint16 camera_number = 0;
    if (PV_FAIL == pl_cam_get_total(&camera_number)) {onError("pl_cam_get_total()"); return;}
    if (camera_number == 0) {onError("camera_number == 0"); return;}
    camera_name.resize(CAM_NAME_LEN);
    if (PV_FAIL == pl_cam_get_name(0, camera_name.data())) {onError("pl_cam_get_name()"); return;}
// open camera
    if (PV_FAIL == pl_cam_open(camera_name.data(), &hCam, OPEN_EXCLUSIVE)) {onError("pl_cam_open()"); return;}
// I skip the whole get device driver version thing here
    if (!initCCDSize()) return;
// I skip the time table thing and not use the initSpeedTable function because the table is there in the cascade512B manual
// set clear mode and cycle number
    int clear_mode = CLEAR_PRE_SEQUENCE;
    setParam(PARAM_CLEAR_MODE, "PARAM_CLEAR_MODE", &clear_mode);
    quint16 clear_cycle_no = 2;
    setParam(PARAM_CLEAR_CYCLES, "PARAM_CLEAR_CYCLES", &clear_cycle_no);
// enable on chip multiplier gain because it is claimed to reduce read noise (if it is possible)
    int shutter_mode = OPEN_PRE_EXPOSURE;
    setParam(PARAM_SHTR_OPEN_MODE, "PARAM_SHTR_OPEN_MODE", &shutter_mode);
//    rs_bool is_frame_transfer;
//    if (!assertParamAvailability(PARAM_FRAME_CAPABLE, "PARAM_FRAME_CAPABLE")) return;
//    pl_get_param(hCam, PARAM_FRAME_CAPABLE, ATTR_CURRENT, &is_frame_transfer);
//    if (is_frame_transfer) {
//        int p_mode = PMODE_FT;
//        setParam(PARAM_PMODE, "PARAM_PMODE", &p_mode);
//    }
    rs_bool can_circ_buffer;
    pl_get_param(hCam, PARAM_CIRC_BUFFER, ATTR_AVAIL, &can_circ_buffer);
    qDebug() << "can use circular buffer" << can_circ_buffer;
    uns32 readout_port = 0;
    setParam(PARAM_READOUT_PORT, "PARAM_READOUT_PORT", &readout_port);
    int16 speed_table_index = 0;
    setParam(PARAM_SPDTAB_INDEX, "PARAM_SPDTAB_INDEX", &speed_table_index);
    int16 pixel_depth = 16;
    setParam(PARAM_BIT_DEPTH, "PARAM_BIT_DEPTH", &pixel_depth);
    uns32 readout_time;
    if (PV_FAIL == pl_get_param(hCam, PARAM_READOUT_TIME, ATTR_CURRENT, &readout_time)) {onError("pl_get_param(PARAM_PAR_SIZE)"); return;}
    qDebug() << "readout time (ns): " << readout_time;
    is_initialized = true;
    qDebug() << "camera initialized";
    startAcquisition();
}

void Camera::rect2rgn(const QRect &source, rgn_type &dest) {
    dest.p1 = source.left();
    dest.p2 = source.right();
    dest.pbin = 1;
    dest.s1 = source.top();
    dest.s2 = source.bottom();
    dest.sbin = 1;
}

void Camera::rgn2rect(const rgn_type &source, QRect &dest) {
    dest.setLeft(source.p1);
    dest.setRight(source.p2);
    dest.setTop(source.s1);
    dest.setBottom(source.s2);
}

void Camera::startAcquisition() {
    if (!is_initialized || is_running) return;
    rgn_type rgn_roi;
    rect2rgn(roi, rgn_roi);
    uns32 stream_size;
    if (PV_FAIL == pl_exp_setup_cont(hCam, 1, &rgn_roi, TIMED_MODE, 40, &stream_size, CIRC_OVERWRITE)) {onError("pl_exp_setup_cont()"); return;}
    frame_size = stream_size / 2;  // sizeof(uint16) = 2
    circ_buffer = new quint16[CIRCULAR_BUFFER_FRAME_NO * frame_size + 1];
    qDebug() << "allocated buffer total " << CIRCULAR_BUFFER_FRAME_NO * stream_size << ", each frame size " << stream_size;
    if (PV_FAIL == pl_exp_start_cont(hCam, circ_buffer, CIRCULAR_BUFFER_FRAME_NO * stream_size)) {onError("pl_exp_start_cont()"); return;}
    is_running = true;
    qDebug() << "camera started";
}

void Camera::stopAcquisition() {
    if (!is_initialized || !is_running) return;
    if (PV_FAIL == pl_exp_stop_cont(hCam, CCS_HALT)) {lookUpError("pl_exp_stop_cont"); return;}
    delete[] circ_buffer;
    is_running = false;
    qDebug() << "camera stopped";
}

bool Camera::assertParamAvailability(quint32 param_id, QString param_name) {
    // make sure the parameter is available for this PVCam model. If not call cleanUp and return false;
    bool is_available;
    if (PV_FAIL == pl_get_param(hCam, param_id, ATTR_AVAIL, &is_available)) {onError("pl_get_param(ATTR_AVAIL)" + param_name); return false;}
    if (!is_available) {onError("parameter is not available" + param_name); return false;}
    return true;
}

bool Camera::setParam(quint32 param_id, QString param_name, void *param_value_ptr) {
    uns16 access_type;
    if (!assertParamAvailability(param_id, param_name)) return false;
    if (PV_FAIL == pl_get_param(hCam, param_id, ATTR_ACCESS, &access_type)) {onError("pl_get_param(" + param_name + ", ATTR_ACCESS)"); return false;}
    if (access_type == ACC_READ_WRITE) {
        if (PV_FAIL == pl_set_param(hCam, param_id, param_value_ptr)) {onError("pl_set_param("+ param_name + ")"); return false;}
        qDebug() << "set " << param_name;
        return true;
    }
    return false;
}

bool Camera::initCCDSize() {
    quint16 resolution_x, resolution_y;
    if (!assertParamAvailability(PARAM_SER_SIZE, "PARAM_SER_SIZE")) return false;
    if (PV_FAIL == pl_get_param(hCam, PARAM_SER_SIZE, ATTR_CURRENT, &resolution_x)) {onError("pl_get_param(PARAM_SER_SIZE)"); return false;}
    if (!assertParamAvailability(PARAM_PAR_SIZE, "PARAM_PAR_SIZE")) return false;
    if (PV_FAIL == pl_get_param(hCam, PARAM_PAR_SIZE, ATTR_CURRENT, &resolution_y)) {onError("pl_get_param(PARAM_PAR_SIZE)"); return false;}
    if (resolution_x < 512) ccd_size.setRight(resolution_x);
    if (resolution_y < 512) ccd_size.setBottom(resolution_y);
    roi = ccd_size;
    return true;
}

bool Camera::initSpeedTable(QVector<QPoint> &indices, QVector<float> &readout_freq, QVector<quint16> &bit_depth) {
    if (!assertParamAvailability(PARAM_READOUT_PORT, "PARAM_READOUT_PORT")) return false;
    if (!assertParamAvailability(PARAM_SPDTAB_INDEX, "PARAM_SPDTAB_INDEX")) return false;
    if (!assertParamAvailability(PARAM_PIX_TIME, "PARAM_PIX_TIME")) return false;
    if (!assertParamAvailability(PARAM_BIT_DEPTH, "PARAM_BIT_DEPTH")) return false;

    quint32 readout_port_no, readout_speed_no;
    quint16 readout_nanoseconds_per_pixel;
    qint16 pixel_depth;
    if (PV_FAIL == pl_get_param(hCam, PARAM_READOUT_PORT, ATTR_COUNT, &readout_port_no)) {onError("pl_get_param(PARAM_READOUT_PORT)"); return false;}
    for (quint32 i = 0; i < readout_port_no; i++) {
        if (PV_FAIL == pl_set_param(hCam, PARAM_READOUT_PORT, &i)) {onError("pl_set_param(PARAM_READOUT_PORT)" + QString::number(i)); return false;}
        if (PV_FAIL == pl_get_param(hCam, PARAM_SPDTAB_INDEX, ATTR_COUNT, &readout_speed_no)) {onError("pl_get_param(PARAM_SPDTAB_INDEX) at port " + QString::number(i)); return false;}
        int16 readout_speed_no_signed = readout_speed_no;
        for (int16 j = 0; j < readout_speed_no_signed; j++) {
            if (PV_FAIL == pl_set_param(hCam, PARAM_SPDTAB_INDEX, &j)) {onError(QString("pl_set_param(PARAM_SPDTAB_INDEX) at port %1, speed: %2").arg(i).arg(j)); return false;}
            if (PV_FAIL == pl_get_param(hCam, PARAM_PIX_TIME, ATTR_CURRENT, &readout_nanoseconds_per_pixel)) {onError(QString("pl_get_param(PARAM_PIX_TIME) at port %1, speed: %2").arg(i).arg(j)); return false;}
            if (PV_FAIL == pl_get_param(hCam, PARAM_BIT_DEPTH, ATTR_CURRENT, &pixel_depth)) {onError(QString("pl_get_param(PARAM_BIT_DEPTH) at port %1, speed: %2").arg(i).arg(j)); return false;}
            indices << QPoint(i, j);
            readout_freq << 1000 / double(readout_nanoseconds_per_pixel);
            bit_depth << pixel_depth;
            qDebug() << "speed: " << i << j << readout_nanoseconds_per_pixel << pixel_depth;
        }
    }
    return true;
}

QVector<QPair<qint32, QByteArray>> Camera::enumerateParameter(quint32 param_id, QString param_name) {
    QVector<QPair<qint32, QByteArray>> result;
    if (!assertParamAvailability(param_id, param_name)) return result;
    quint32 param_count;
    pl_get_param(hCam, param_id, ATTR_COUNT, &param_count);
    for (quint32 i=0; i<param_count;i++) {
        QByteArray enum_name;
        uns32 enum_length;
        pl_enum_str_length(hCam, param_id, i, &enum_length);
        enum_name.reserve(enum_length);
        int32 enum_value;
        pl_get_enum_param(hCam, param_id, i, &enum_value, enum_name.data(), enum_length);
        result << QPair<qint32, QByteArray>(qint32(enum_value), enum_name);
        qDebug() << "parameter " << param_name << ": value=" << enum_value << ", name=" << enum_name;
    }
    return result;
}

void Camera::onError(QString function_name) {
    lookUpError(function_name);
    cleanup();
}

void Camera::lookUpError(QString function_name) {
    QByteArray camera_error_message;
    camera_error_message.resize(256);
    pl_error_message(pl_error_code(), camera_error_message.data());
    emit(raiseError(function_name + ": " + QString::fromLocal8Bit(camera_error_message)));
    qDebug() << "error emitted";
}

void Camera::cleanup() {
    qDebug() << "cleanup";
    if (!needs_clean_up) return;
    if (is_running) stopAcquisition();
    if (PV_FAIL == pl_exp_uninit_seq()) lookUpError("pl_exp_uninit_seq()");
    is_initialized = false;
// pl_pvcam_uninit() automatically calls pl_cam_close()
    if (PV_FAIL == pl_pvcam_uninit()) lookUpError("pl_pvcam_uninit()");
    needs_clean_up = false;
}

void Camera::setROI(const QRect &roi_in) {
    if (!params->lock()) {
        emit(raiseError("ROI cannot be changed duing recording!"));
    } else {
        if (is_running) {
            stopAcquisition();
            roi = roi_in;
            startAcquisition();
        } else roi = roi_in;
        params->unlock();
    }
}

void Camera::captureFrame(quint64 timestamp, double phase) {
    if (!is_running || !is_initialized) return;
    qint16 camera_status;
    uns32 byte_count, buffer_count;
    pl_exp_check_cont_status(hCam, &camera_status, &byte_count, &buffer_count);
    qDebug() << "circ_buffer_status" << byte_count << buffer_count << camera_status;
    if (camera_status == FRAME_AVAILABLE) {
        QVector<quint16> image(frame_size);
        void *temp_data_ptr;
        if (PV_FAIL == pl_exp_get_oldest_frame(hCam, &temp_data_ptr)) {onError("pl_exp_get_oldest_frame"); return;}
        quint16 *data_ptr = static_cast<quint16 *>(temp_data_ptr);
        std::copy(data_ptr, data_ptr + frame_size - 1, image.data());
        if (PV_FAIL == pl_exp_unlock_oldest_frame(hCam)) {onError("pl_exp_unlock_oldest_frame"); return;}
        debug_counter++;
        qDebug() << "emitted " << debug_counter << " frame at " << timestamp << QTime::currentTime().msec();
        emit(yieldFrame(timestamp, phase, image, roi));
    }
}

void Camera::checkTemp() {
    // get temperature
    if (!is_initialized) return;
    if (!assertParamAvailability(PARAM_TEMP, "PARAM_TEMP")) return;
    int16 temp;
    if (PV_FAIL == pl_get_param(hCam, PARAM_TEMP, ATTR_CURRENT, &temp)) {onError("PARAM_TEMP"); return;}
    emit(yieldTemperature(int(temp/100)));
}

QRect Camera::getROI() const {
    return roi;
}

bool Camera::isReady() const {
    return is_running;
}
