#include <master.h>
#include <pvcam.h>
#include <QDebug>
#include <QTime>
#include "recordparams.h"
#include "regionofinterest.h"

#include "camera.h"

Camera::Camera(QObject *parent) : QObject(parent),
    callback_registered(false), is_initialized(false), needs_clean_up(false), is_running(false) {
    params = RecordParams::getParams();
    roi = RegionOfInterest::getRegion();
}

Camera::~Camera() {
    cleanup();
}

void Camera::init() {
    if (is_initialized) {emit(raiseError("Can't repeatedly init camera")); return;}
// initialize PVcam
    if (PV_FAIL == pl_pvcam_init()) {onError("pl_pvcam_init()"); return;}
    needs_clean_up = true;
    quint16 pvcam_version;
    if (PV_FAIL == pl_pvcam_get_ver(&pvcam_version)) {onError("pl_pvcam_get_ver()"); return;}
    qDebug() << "PVCAM version" << ((pvcam_version & 0xFF00) >> 8) << ((pvcam_version & 0xF0) >> 4) << (pvcam_version & 0xF);
    if (PV_FAIL == pl_exp_init_seq()) {onError("pl_exp_init_seq()"); return;}
    qint16 camera_number = 0;
    if (PV_FAIL == pl_cam_get_total(&camera_number)) {onError("pl_cam_get_total()"); return;}
    if (camera_number == 0) {onError("camera_number == 0"); return;}
    camera_name.resize(CAM_NAME_LEN);
    if (PV_FAIL == pl_cam_get_name(0, camera_name.data())) {onError("pl_cam_get_name()"); return;}
    // open camera
    if (PV_FAIL == pl_cam_open(camera_name.data(), &hCam, OPEN_EXCLUSIVE)) {onError("pl_cam_open()"); return;}
    if (!initCameraInfo()) return;

    // set frame transfer mode
    rs_bool is_frame_transfer;
    if (!assertParamAvailability(PARAM_FRAME_CAPABLE, "PARAM_FRAME_CAPABLE")) return;
    pl_get_param(hCam, PARAM_FRAME_CAPABLE, ATTR_CURRENT, &is_frame_transfer);
    if (is_frame_transfer) {
        int p_mode = PMODE_FT;
        setParam(PARAM_PMODE, "PARAM_PMODE", &p_mode);
    }

    // set readout speed and bit depth
    uns32 readout_port = 0;
    if (!setParam(PARAM_READOUT_PORT, "PARAM_READOUT_PORT", &readout_port)) return;
    int16 speed_table_index = 0;
    if (!setParam(PARAM_SPDTAB_INDEX, "PARAM_SPDTAB_INDEX", &speed_table_index)) return;
    int16 pixel_depth;
    if (PV_FAIL == pl_get_param(hCam, PARAM_BIT_DEPTH, ATTR_CURRENT, &pixel_depth)) {onError("pl_get_param(PARAM_BIT_DEPTH"); return;}
    qDebug() << "pixel depth at: " << pixel_depth;

    if (!initCCDSize()) return;
    // set clear mode and cycle number
    int clear_mode = CLEAR_PRE_SEQUENCE;
    setParam(PARAM_CLEAR_MODE, "PARAM_CLEAR_MODE", &clear_mode);
    quint16 clear_cycle_no = 2;
    setParam(PARAM_CLEAR_CYCLES, "PARAM_CLEAR_CYCLES", &clear_cycle_no);
    int shutter_mode = OPEN_PRE_EXPOSURE;
    setParam(PARAM_SHTR_OPEN_MODE, "PARAM_SHTR_OPEN_MODE", &shutter_mode);

    // if temperature higher than -20 abort
//    int16 temperture;
//    if (PV_FAIL == pl_get_param(hCam, PARAM_TEMP, ATTR_CURRENT, &temperture)) {onError("pl_get_param(PARAM_TEMP)"); return;}
//    if (temperture > -2000) {onError("temperature too high"); return;}

    uns32 readout_time;
    if (PV_FAIL == pl_get_param(hCam, PARAM_READOUT_TIME, ATTR_CURRENT, &readout_time)) {onError("pl_get_param(PARAM_PAR_SIZE)"); return;}
    qDebug() << "readout time (ns): " << readout_time;
    if (PV_FAIL == pl_cam_register_callback_ex(hCam, PL_CALLBACK_EOF, camCallback, this)) {onError("pl_cam_register_callback_ex"); return;}
    callback_registered = true;
    is_initialized = true;
    qDebug() << "camera initialized";
    startAcquisition();
}

bool Camera::initCameraInfo() {
    quint16 driver_version;
    if (PV_FAIL == pl_get_param(hCam, PARAM_DD_VERSION, ATTR_CURRENT, &driver_version)) {onError("pl_get_param(PARAM_DD_VERSION)"); return false;}
    qDebug() << "Driver Version" << ((driver_version & 0xFF00) >> 8) << ((driver_version & 0xF0) >> 4) << (driver_version & 0xF);
    QByteArray ccd_name;
    ccd_name.resize(CCD_NAME_LEN);
    if (PV_FAIL == pl_get_param(hCam, PARAM_CHIP_NAME, ATTR_CURRENT, ccd_name.data())) {onError("pl_get_param(PARAM_CHIP_NAME"); return false;}
    qDebug() << "CCD Chip name" << ccd_name;
    // firmware version
    quint16 firmware_version;
    if (PV_FAIL == pl_get_param(hCam, PARAM_CAM_FW_VERSION, ATTR_CURRENT, &firmware_version)) {onError("pl_get_param(PARAM_CAM_FW_VERSION)"); return false;}
    qDebug() << "Firmware version:" << ((firmware_version & 0xFF00) >> 8) << (firmware_version & 0xFF);
    return true;
}

void Camera::startAcquisition() {
    if (!is_initialized || is_running) return;
    rgn_type rgn_roi = roi->getRGN();
    qDebug() << "rect set" << rgn_roi.p1 << rgn_roi.p2 << rgn_roi.pbin << rgn_roi.s1 << rgn_roi.s2 << rgn_roi.sbin;
    quint32 stream_size;
    if (PV_FAIL == pl_exp_setup_cont(hCam, 1, &rgn_roi, TIMED_MODE, params->getExposureTime(), reinterpret_cast<uns32_ptr>(&stream_size), CIRC_OVERWRITE)) {onError("pl_exp_setup_cont()"); return;}
    frame_size = stream_size / sizeof(quint16);  // sizeof(uint16) = 2
    buffer = QVector<quint16>(CIRCULAR_BUFFER_FRAME_NO * frame_size);
    if (PV_FAIL == pl_exp_start_cont(hCam, buffer.data(), CIRCULAR_BUFFER_FRAME_NO * frame_size)) {onError("pl_exp_start_cont()"); return;}
    is_running = true;
    qDebug() << "camera started";
}

void Camera::stopAcquisition() {
    if (!is_initialized || !is_running) return;
    if (PV_FAIL == pl_exp_stop_cont(hCam, CCS_HALT)) {lookUpError("pl_exp_stop_cont"); return;}
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
        if (PV_FAIL == pl_set_param(hCam, param_id, param_value_ptr)) {onError("pl_set_param(" + param_name + ")"); return false;}
        qDebug() << "set " << param_name;
        return true;
    }
    return true;
}

bool Camera::initCCDSize() {
    quint16 resolution_x, resolution_y;
    if (!assertParamAvailability(PARAM_SER_SIZE, "PARAM_SER_SIZE")) return false;
    if (PV_FAIL == pl_get_param(hCam, PARAM_SER_SIZE, ATTR_CURRENT, &resolution_x)) {onError("pl_get_param(PARAM_SER_SIZE)"); return false;}
    if (!assertParamAvailability(PARAM_PAR_SIZE, "PARAM_PAR_SIZE")) return false;
    if (PV_FAIL == pl_get_param(hCam, PARAM_PAR_SIZE, ATTR_CURRENT, &resolution_y)) {onError("pl_get_param(PARAM_PAR_SIZE)"); return false;}
    roi->setROILimit(QRect(0, 0, resolution_x, resolution_y));
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
    for (quint32 i = 0; i < param_count; i++) {
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
    if (callback_registered) {pl_cam_deregister_callback(hCam, PL_CALLBACK_EOF); callback_registered = false;}
    if (PV_FAIL == pl_exp_uninit_seq()) lookUpError("pl_exp_uninit_seq()");
    is_initialized = false;
// pl_pvcam_uninit() automatically calls pl_cam_close()
    if (PV_FAIL == pl_pvcam_uninit()) lookUpError("pl_pvcam_uninit()");
    needs_clean_up = false;
}

void Camera::setROI() {
    if (roi->isLocked()) {
        emit(raiseError("ROI cannot be changed during recording!"));
    } else if (is_running) {
        stopAcquisition();
        startAcquisition();
    }
}

void Camera::captureFrame() {
    qint16 status;
    quint32 byte_count, buffer_count;
    if (PV_FAIL == pl_exp_check_cont_status(hCam, &status, reinterpret_cast<uns32_ptr>(&byte_count), reinterpret_cast<uns32_ptr>(&buffer_count))) {onError("pl_exp_check_cont_status"); return;}
    if (status == READOUT_FAILED) {onError("readout failed"); return;}
    if (status != READOUT_COMPLETE) {qDebug() << "readout attempt too early"; return;}
    QVector<quint16> image(frame_size);
    quint16 *frame_address; if (PV_FAIL == pl_exp_get_latest_frame(hCam, reinterpret_cast<void **>(&frame_address))) {onError("pl_exp_get_latest_frame"); return;} std::copy(frame_address, frame_address + frame_size - 1, image.data());
    emit(yieldFrame(QTime::currentTime().msecsSinceStartOfDay(), image));
}

void Camera::checkTemp() {
    /* get temperature
     */
    if (!is_initialized || params->isLocked()) return;
    stopAcquisition();
    int16 temp;
    if (PV_FAIL == pl_get_param(hCam, PARAM_TEMP, ATTR_CURRENT, &temp)) {onError("PARAM_TEMP"); return;}
    startAcquisition();
    emit(yieldTemperature(int(temp / 100)));
}

bool Camera::isReady() const {
    return is_running;
}

void camCallback(void *content) {
    static_cast<Camera *>(content)->captureFrame();
}
