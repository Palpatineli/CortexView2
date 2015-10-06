#include <qmath.h>
#include <QSettings>
#include <QVariant>

#include "NIDAQmx.h"
#include "nidaqbox.h"

NIDaqBox::NIDaqBox(QObject *parent) : QObject(parent),
    previous_diode(0), previous_cam(0),
    previous_diode_diff(0), previous_cam_diff(0),
    diode_rising(false), current_time(0),
    last_diode_signal_time(0), last_diode_signal_phase(0),
    phase_per_tick(0), phase_per_pulse(0),
    hTask(NULL), is_initialized(false), error_id(0){
}

NIDaqBox::~NIDaqBox() {
    cleanUp(0);
}

void NIDaqBox::init(NIDaqBox *stream) {
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

    error_id = DAQmxCreateTask("", &hTask);
    if (error_id < 0) cleanUp(error_id);
    error_id = DAQmxCreateAIVoltageChan(hTask, device_string.toLocal8Bit(), "", DAQmx_Val_Cfg_Default, -10.0, 10.0, DAQmx_Val_Volts, NULL);
    if (error_id < 0) cleanUp(error_id);
    error_id = DAQmxCfgSampClkTiming(hTask, "", 1000.0, DAQmx_Val_Rising, DAQmx_Val_ContSamps, 0);
    if (error_id < 0) cleanUp(error_id);
    error_id = DAQmxRegisterSignalEvent(hTask, DAQmx_Val_SampleCompleteEvent, 0, signalEventCallback, stream);
//    error = DAQmxRegisterEveryNSamplesEvent(hTask, DAQmx_Val_Acquired_Into_Buffer, 1, 0, signalEventCallback, stream);
    if (error_id < 0) cleanUp(error_id);
    error_id = DAQmxStartTask(hTask);
    is_running = true;
    if (error_id < 0) cleanUp(error_id);
}

void NIDaqBox::cleanUp(int32 error_id) {
    if (hTask) {  // if task is already created
        if (error_id) {  // raise error if returned in error
            char error_message_buffer[2048] = {'\0'};
            DAQmxGetExtendedErrorInfo(error_message_buffer, 2048);
            error_message = QString::fromLocal8Bit(error_message_buffer);
            emit(DAQerror(error_message));
        }
        if (is_running) DAQmxStopTask(hTask);
        DAQmxRegisterSignalEvent(hTask, DAQmx_Val_SampleCompleteEvent, 0, NULL, NULL);
        DAQmxClearTask(hTask);
        hTask = 0;
    }
}

void NIDaqBox::setPhaseParams(quint32 frame_no, double cycle_time) {
    phase_per_tick = (2 * M_PI) / (cycle_time * NI_FREQ);
    // due to diode accumulation the monitor can only send one signal every six to nine frames ~75Hz.
    phase_per_pulse = (12* M_PI) / (frame_no);
}

void NIDaqBox::readSample(TaskHandle hTask) {
/* Read every voltage sample from NIDaq box as it comes in, find
 * 1) the onset of pvcam expose out
 * 2) the onset and height of diode input rising edge
*/
    error_id = DAQmxReadAnalogF64(hTask, 1, 0.05, DAQmx_Val_GroupByScanNumber, tempBuffer, 2, &read_no, NULL);
    if (error_id < 0) {
        char errBuff[2048] = {'\0'};
        DAQmxGetExtendedErrorInfo(errBuff, 2048);
        QString error_message = QString::fromLocal8Bit(errBuff);
        emit(raiseError(error_message));
        return;
    }
    if (read_no > 0) {
        cam_diff = tempBuffer[1] - previous_cam;
        if (cam_diff > 0 && previous_cam_diff <= 0) emit(cameraFrameReady(current_time, phase_per_tick * (current_time - last_diode_signal_time) + last_diode_signal_phase));
        previous_cam = tempBuffer[1];
        previous_cam_diff = cam_diff;
        diode_diff = tempBuffer[0] - previous_diode;
        if (diode_diff > 0 && previous_diode_diff <= 0 && !diode_rising) {
            current_diode_signal_time = current_time;
            diode_onset_level = previous_diode;
            diode_rising = true;
        } else if (diode_diff < 0 && previous_diode_diff >= 0 && diode_rising) {
            last_diode_signal_time = current_diode_signal_time;
            double signal = previous_diode - diode_onset_level;
            quint8 level = -1;
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

int32 CVICALLBACK signalEventCallback (TaskHandle hTask, int32 /*eventType*//*, uInt32 *//*nSamples*/, void *callbackData) {
/* Move to NIDaqSignalStream, which belongs to NIDaqBox but not initialized in the QThread
*/
    (static_cast<NIDaqBox*>(callbackData))->readSample(hTask);
    return 0;
}
