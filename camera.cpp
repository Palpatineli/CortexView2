#include "master.h"
#include "pvcam.h"

#include "camera.h"

Camera::Camera(QObject *parent) : QObject(parent),
    needs_clean_up(false), ccd_size(0, 0, 512, 512), roi(0, 0, 1, 1, 511, 511) {
    camera_name.resize(CAM_NAME_LEN);
}

Camera::~Camera() {
    if (needs_clean_up) cleanUp("Camera::~Camera()");
}

void Camera::init() {
    if (needs_clean_up) return;
// initialize PVcam
    if (PV_FAIL == pl_pvcam_init()) {cleanUp("pl_pvcam_init()"); return;}
    needs_clean_up = true;
    if (PV_FAIL == pl_exp_init_seq()) {cleanUp("pl_exp_init_seq()"); return;}
    qint16 camera_number;
    if (PV_FAIL == pl_cam_get_total(&camera_number)) {cleanUp("pl_cam_get_total()"); return;}
    if (camera_number == 0) {cleanUp("camera_number == 0"); return;}
    if (PV_FAIL == pl_cam_get_name(0, camera_name.data())) {cleanUp("pl_cam_get_name()"); return;}
// open camera
    if (PV_FAIL == pl_cam_open(camera_name.data(), &hCam, OPEN_EXCLUSIVE)) {cleanUp("pl_cam_open()"); return;}
// I skip the whole get device driver version thing here
    if (!initCCDSize()) return;
    rect2rgn(ccd_size, roi);
// I skip the time table thing and not use the initSpeedTable function because the table is there in the cascade512B manual
// set clear mode and cycle number
    if (!assertParamAvailability(PARAM_CLEAR_MODE)) return;
    if (!assertParamAvailability(PARAM_CLEAR_CYCLES)) return;
    int clear_mode = CLEAR_PRE_SEQUENCE;
    quint16 clear_cycle_no = 2;
    if (PV_FAIL == pl_set_param(hCam, PARAM_CLEAR_MODE, &clear_mode)) {cleanUp("pl_set_param(PARAM_CLEAR_MODE)"); return;}
    if (PV_FAIL == pl_set_param(hCam, PARAM_CLEAR_CYCLES, &clear_cycle_no)) {cleanUp("pl_set_param(PARAM_CLEAR_CYCLES)"); return;}
// enable on chip multiplier gain because it is claimed to reduce read noise
    if (!assertParamAvailability(PARAM_GAIN_MULT_ENABLE)) return;
    bool gain_multiplier_enable = true;
    if (PV_FAIL == pl_set_param(hCam, PARAM_GAIN_MULT_ENABLE, &gain_multiplier_enable)) {cleanUp("pl_set_param(PARAM_GAIN_MULT_ENABLE)"); return;}
    emit(cameraReady());
    is_initialized = true;
}

void Camera::rect2rgn(const QRect & source, rgn_type &dest) {
    dest.p1 = source.left();
    dest.p2 = source.right();
    dest.s1 = source.top();
    dest.s2 = source.bottom();
}

void Camera::rgn2rect(const rgn_type & source, QRect &dest) {
    dest.setLeft(source.p1);
    dest.setRight(source.p2);
    dest.setTop(source.s1);
    dest.setBottom(source.s2);
}

void Camera::startAcquisition() {
    if (is_running) return;
    if (PV_FAIL == pl_exp_setup_cont(hCam, 1, &roi, TIMED_MODE, 40, &stream_size, CIRC_NO_OVERWRITE)) {cleanUp("pl_exp_setup_cont()"); return;}
    buffer_size = stream_size / sizeof(quint16);;
    circ_buffer.resize(buffer_size * CIRCULAR_BUFFER_FRAME_NO);
    if (PV_FAIL == pl_exp_start_cont(hCam, circ_buffer.data(), buffer_size * CIRCULAR_BUFFER_FRAME_NO)) {cleanUp("pl_exp_start_cont()"); return;}
    is_running = true;
    emit(cameraStarted());
}

void Camera::stopAcquisition() {
    if (!is_running) return;
    if (PV_FAIL == pl_exp_stop_cont(hCam, CCS_HALT)) {lookUpError("pl_exp_stop_cont"); return;}
    is_running = false;
    emit(cameraStopped());
}

bool Camera::assertParamAvailability(quint32 param_id) {
    // make sure the parameter is available for this PVCam model. If not call cleanUp and return false;
    bool is_available;
    if (PV_FAIL == pl_get_param(hCam, param_id, ATTR_AVAIL, &is_available)) {cleanUp("pl_get_param(ATTR_AVAIL)" + QString::number(param_id)); return false;}
    if (!is_available) {cleanUp("parameter is not available" + QString::number(param_id)); return false;}
    return true;
}

bool Camera::initCCDSize() {
    quint16 resolution_x, resolution_y;
    if (!assertParamAvailability(PARAM_SER_SIZE)) return false;
    if (PV_FAIL == pl_get_param(hCam, PARAM_SER_SIZE, ATTR_CURRENT, &resolution_x)) {cleanUp("pl_get_param(PARAM_SER_SIZE)"); return false;}
    if (!assertParamAvailability(PARAM_PAR_SIZE)) return false;
    if (PV_FAIL == pl_get_param(hCam, PARAM_PAR_SIZE, ATTR_CURRENT, &resolution_y)) {cleanUp("pl_get_param(PARAM_PAR_SIZE)"); return false;}
    if (resolution_x < 512) ccd_size.setRight(resolution_x);
    if (resolution_y < 512) ccd_size.setBottom(resolution_y);
    return true;
}

bool Camera::initSpeedTable(QVector<QPoint> &indices, QVector<float> &readout_freq, QVector<quint16> &bit_depth) {
    if (!assertParamAvailability(PARAM_READOUT_PORT)) return false;
    if (!assertParamAvailability(PARAM_SPDTAB_INDEX)) return false;
    if (!assertParamAvailability(PARAM_PIX_TIME)) return false;
    if (!assertParamAvailability(PARAM_BIT_DEPTH)) return false;

    quint32 readout_port_no, readout_speed_no;
    quint16 readout_nanoseconds_per_pixel;
    qint16 pixel_depth;
    if (PV_FAIL == pl_get_param(hCam, PARAM_READOUT_PORT, ATTR_COUNT, &readout_port_no)) {cleanUp("pl_get_param(PARAM_READOUT_PORT)"); return false;}
    for (quint32 i = 0; i < readout_port_no; i++) {
        if (PV_FAIL == pl_set_param(hCam, PARAM_READOUT_PORT, &i)) {cleanUp("pl_set_param(PARAM_READOUT_PORT)" + QString::number(i)); return false;}
        if (PV_FAIL == pl_get_param(hCam, PARAM_SPDTAB_INDEX, ATTR_COUNT, &readout_speed_no)) {cleanUp("pl_get_param(PARAM_SPDTAB_INDEX) at port " + QString::number(i)); return false;}
        int16 readout_speed_no_signed = readout_speed_no;
        for (int16 j = 0; j < readout_speed_no_signed; j++) {
            if (PV_FAIL == pl_set_param(hCam, PARAM_SPDTAB_INDEX, &j)) {cleanUp(QString("pl_set_param(PARAM_SPDTAB_INDEX) at port %1, speed: %2").arg(i).arg(j)); return false;}
            if (PV_FAIL == pl_get_param(hCam, PARAM_PIX_TIME, ATTR_CURRENT, &readout_nanoseconds_per_pixel)) {cleanUp(QString("pl_get_param(PARAM_PIX_TIME) at port %1, speed: %2").arg(i).arg(j)); return false;}
            if (PV_FAIL == pl_get_param(hCam, PARAM_BIT_DEPTH, ATTR_CURRENT, &pixel_depth)) {cleanUp(QString("pl_get_param(PARAM_BIT_DEPTH) at port %1, speed: %2").arg(i).arg(j)); return false;}
            indices << QPoint(i, j);
            readout_freq << 1000 / double(readout_nanoseconds_per_pixel);
            bit_depth << pixel_depth;
        }
    }
    return true;
}

void Camera::cleanUp(QString function_name) {
    lookUpError(function_name);
    if (!needs_clean_up) return;
    if (is_running) stopAcquisition();
    if (PV_FAIL == pl_exp_uninit_seq()) lookUpError("pl_exp_uninit_seq()");
    is_initialized = false;
// pl_pvcam_uninit() automatically calls pl_cam_close()
    if (PV_FAIL == pl_pvcam_uninit()) lookUpError("pl_pvcam_uninit()");
    needs_clean_up = false;
}

void Camera::lookUpError(QString function_name) {
    QByteArray camera_error_message;
    camera_error_message.resize(256);
    pl_error_message(pl_error_code(), camera_error_message.data());
    emit(raiseError(function_name + ": " + QString::fromLocal8Bit(camera_error_message)));
}

void Camera::setROI(const QRect &roi_in) {
    if (is_recording) {
        emit(raiseError("ROI cannot be changed duing recording!"));
    } else if (is_running) {
        stopAcquisition();
        rect2rgn(roi_in, roi);
        startAcquisition();
    } else rect2rgn(roi_in, roi);
}

void Camera::setRecording(bool is_recording_in) {
    if (!is_running) return;
    is_recording = is_recording_in;
}

void Camera::captureFrame(quint64 timestamp, double phase) {
    if (!is_running) return;
    qint16 camera_status;
    uns32 byte_count, buffer_count;
    pl_exp_check_cont_status(hCam, &camera_status, &byte_count, &buffer_count);
    if (camera_status == FRAME_AVAILABLE) {
        QVector<quint16> image(buffer_size);
        QRect roi;
        rgn2rect(roi, roi);
        void* temp_data_ptr;
        pl_exp_get_latest_frame(hCam, &temp_data_ptr);
        quint16* data_ptr = static_cast<quint16*>(temp_data_ptr);
        std::copy(data_ptr, data_ptr+buffer_size, image.data());
        emit(yieldFrame(timestamp, phase, image, roi));
    }
}
