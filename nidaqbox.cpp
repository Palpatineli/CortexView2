#include <qmath.h>
#include <QSettings>
#include <QVariant>
#include <QDebug>

#include "NIDAQmx.h"
#include "nidaqbox.h"
#include "recordparams.h"

NIDaqBox::NIDaqBox(QObject *parent) : QObject(parent),
    is_initialized(false), cam_is_low(true) {
    params = RecordParams::getParams();
}

NIDaqBox::~NIDaqBox() {
    cleanup();
}

void NIDaqBox::init() {
    QString device_string;

    // load or initalize diode steps
    QSettings settings;
    if (settings.contains("diode_levels")) {
        QList<QVariant> tempList = settings.value("diode_levels").toList();
        QVariant item;
        foreach (item, tempList) diode_levels << item.toDouble();
    } else {  // default values
        diode_levels << 0.05 << 0.30074 << 0.51133 << 0.72274 << 0.93121 << 1.134 << 1.35;
        QVariantList tempList;
        foreach (const double item, diode_levels) tempList << QVariant(item);
        settings.setValue("diode_levels", tempList);
    }

    // load or initialize device name
    if (settings.contains("NIDAQ_device_name")) {
        device_string = settings.value("NIDAQ_device_name").toString();\
    } else {
        device_string = "cDAQ1Mod1/ai0:1";  // first one is diode input, second one is camera exposure out input
        settings.setValue("NIDAQ_device_name", device_string);
    }

    if (0 > DAQmxCreateTask("", &hTask)) {onError(); return;}
    if (0 > DAQmxCreateAIVoltageChan(hTask, device_string.toLocal8Bit(), "", DAQmx_Val_Cfg_Default, -10.0, 10.0, DAQmx_Val_Volts, NULL)) {onError(); return;}
    if (0 > DAQmxCfgSampClkTiming(hTask, "", 1000.0, DAQmx_Val_Rising, DAQmx_Val_ContSamps, 0)) {onError(); return;}
    if (0 > DAQmxRegisterSignalEvent(hTask, DAQmx_Val_SampleCompleteEvent, 0, signalEventCallback, this)) {onError(); return;}
    if (0 > DAQmxStartTask(hTask)) {onError(); return;}
    reset();
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

void NIDaqBox::reset() {
    phase_per_tick = (2 * M_PI) / (params->getPeriod_in_seconds() * NI_FREQ);
    // due to diode accumulation the monitor can only send one signal every six to nine frames ~75Hz.
    phase_per_pulse = (2 * frames_per_pulse * M_PI) / (params->getPeriod_in_frames());
    current_phase = -1;
    previous_diode = 0;
    previous_diode_diff = 0;
    last_diode_signal_time = 0;
    last_diode_signal_phase = 0;
    diode_rising = false;
    current_time = 0;
}

void NIDaqBox::readSample(TaskHandle hTask) {
/* Read every voltage sample from NIDaq box as it comes in, find
 * 1) the onset of pvcam expose out
 * 2) the onset and height of diode input rising edge
*/
    if (0 > DAQmxReadAnalogF64(hTask, 1, 0.05, DAQmx_Val_GroupByScanNumber, tempBuffer, 2, &read_no, NULL)) {lookupError(); return;}
    if (read_no > 0) {
        if (tempBuffer[1] > 2.5) {  // low = 0, high = 5
            if (cam_is_low) {
                emit(cameraFrameReady(current_time, phase_per_tick * (current_time - last_diode_signal_time) + last_diode_signal_phase));
                cam_is_low = false;
            }
        } else {
            if (!cam_is_low) {
                cam_is_low = true;
            }
        }

        diode_diff = tempBuffer[0] - previous_diode;
        if (diode_diff > 0 && previous_diode_diff <= 0 && !diode_rising) {
            current_diode_signal_time = current_time;
            diode_onset_level = previous_diode;
            diode_rising = true;
        } else if (diode_diff < 0 && previous_diode_diff >= 0 && diode_rising) {
            last_diode_signal_time = current_diode_signal_time;
            double signal = previous_diode - diode_onset_level;
            quint8 level = -1;
            // search for the level (category) of diode_signal
            for (int i=0; i<diode_levels.size() && signal > diode_levels.at(i); i++) level++;
            emit(yieldDiodeSignal(current_diode_signal_time, signal));
            if (level == 1) {
                last_diode_signal_phase += phase_per_pulse;
            } else if (level == 5) {
                last_diode_signal_phase = 0;
            }
            diode_rising = false;
        }
        previous_diode = tempBuffer[0];
        previous_diode_diff = diode_diff;
        current_time++;
    }
}

bool NIDaqBox::isReady() {
    return is_initialized;
}

int32 CVICALLBACK signalEventCallback (TaskHandle hTask, int32 /*eventType*//*, uInt32 *//*nSamples*/, void *callbackData) {
/* Move to NIDaqSignalStream, which belongs to NIDaqBox but not initialized in the QThread
*/
    (static_cast<NIDaqBox*>(callbackData))->readSample(hTask);
    return 0;
}
