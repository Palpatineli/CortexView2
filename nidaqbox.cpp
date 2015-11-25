#include <QDebug>
#include <QSettings>
#include <QTime>
#include <QVariant>

#include "NIDAQmx.h"
#include "nidaqbox.h"
#include "recordparams.h"

NIDaqBox::NIDaqBox(QObject *parent) : QObject(parent),
    is_initialized(false), cam_is_high(false), diode_is_rising(false),
    previous_diode(10), current_time(0) {
    params = RecordParams::getParams();
}

NIDaqBox::~NIDaqBox() {
    cleanup();
}

void NIDaqBox::init() {
    QString device_string;

    QSettings settings;
    // load or initialize device name
    if (settings.contains("NIDAQ_device_name")) {
        device_string = settings.value("NIDAQ_device_name").toString(); \
    } else {
        device_string = "cDAQ1Mod1/ai0:1";  // first one is diode input, second one is camera exposure out input
        settings.setValue("NIDAQ_device_name", device_string);
    }

    if (0 > DAQmxCreateTask("", &hTask)) {onError(); return;}
    if (0 > DAQmxCreateAIVoltageChan(hTask, device_string.toLocal8Bit(), "", DAQmx_Val_Cfg_Default, -10.0, 10.0, DAQmx_Val_Volts, NULL)) {onError(); return;}
    if (0 > DAQmxCfgSampClkTiming(hTask, "", 1000.0, DAQmx_Val_Rising, DAQmx_Val_ContSamps, 0)) {onError(); return;}
    if (0 > DAQmxRegisterSignalEvent(hTask, DAQmx_Val_SampleCompleteEvent, 0, signalEventCallback, this)) {onError(); return;}
    if (0 > DAQmxStartTask(hTask)) {onError(); return;}
    is_initialized = true;
    qDebug() << "NIDaq box initialized";
}

void NIDaqBox::onError() {
    lookupError();
    cleanup();
}

void NIDaqBox::lookupError() {
    char error_message_buffer[2048] = {'\0'};
    DAQmxGetExtendedErrorInfo(error_message_buffer, 2048);
    QString error_message = QString::fromLocal8Bit(error_message_buffer);
    emit(raiseError(error_message));
}

void NIDaqBox::cleanup() {
    if (!hTask) return;  // if task is not created we have nothing to cleanup
    if (is_initialized) {
        if (0 > DAQmxStopTask(hTask)) lookupError();
        is_initialized = false;
    }
    if (0 > DAQmxRegisterSignalEvent(hTask, DAQmx_Val_SampleCompleteEvent, 0, NULL, NULL)) lookupError();
    if (0 > DAQmxClearTask(hTask)) lookupError();
    hTask = 0;
}

void NIDaqBox::readSample(TaskHandle hTask) {
    /* Read every voltage sample from NIDaq box as it comes in, find
     * 1) the offset of pvcam expose out
     *      the camera outputs high when it is during exposure, and low otherwise
     * 2) the onset and height of diode input rising edge
    */
    int32 read_no;
    if (0 > DAQmxReadAnalogF64(hTask, 1, 0.05, DAQmx_Val_GroupByScanNumber, temp_buffer, 2, &read_no, NULL)) {lookupError(); return;}
    if (read_no <= 0) {onError(); return;}  // if read nothing
    if (temp_buffer[1] < 4.5 && cam_is_high) {  // channel 1 is expose out from camera. low = 0, high = 5
        cam_is_high = false;
    } else if (temp_buffer[1] > 4.5 && !cam_is_high) {
        cam_is_high = true;
        emit(yieldCameraSignal(current_time, QTime::currentTime().msecsSinceStartOfDay()));
    }

    if (temp_buffer[0] - previous_diode > 0.005) {
        if (!diode_is_rising) {
            diode_is_rising = true;
            last_diode_signal_time = current_time;
            diode_baseline = previous_diode;
        }
    } else {
        if (diode_is_rising) {
            diode_is_rising = false;
            double signal = previous_diode - diode_baseline;
            if (signal > 0.05) emit(yieldDiodeSignal(last_diode_signal_time, signal));
        }
    }
    previous_diode = temp_buffer[0];
    current_time++;
}

bool NIDaqBox::isReady() {
    return is_initialized;
}

int32 CVICALLBACK signalEventCallback (TaskHandle hTask, int32 /*eventType*//*, uInt32 *//*nSamples*/, void *callbackData) {
    /* Move to NIDaqSignalStream, which belongs to NIDaqBox but not initialized in the QThread
    */
    (static_cast<NIDaqBox *>(callbackData))->readSample(hTask);
    return 0;
}
