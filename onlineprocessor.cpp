#include <qmath.h>
#include <cmath>
#include <QColor>
#include <QDebug>
#include <QImageWriter>
#include <QSettings>
#include "recordparams.h"
#include "regionofinterest.h"
#include "onlineprocessor.h"

void polar2rgb(double angle, int value, uchar *argb_ptr) {
    double ff;
    uchar t, q;
    long i;
    argb_ptr[0] = 255;

    if(angle >= 360.0) angle = 0.0;
    angle /= 60.0;
    i = (long)angle;
    ff = angle - i;
    q = value * (1.0 - ff);
    t = value * ff;

    switch(i) {
    case 0:
        argb_ptr[1] = value;
        argb_ptr[2] = t;
        argb_ptr[3] = 0;
        break;
    case 1:
        argb_ptr[1] = q;
        argb_ptr[2] = value;
        argb_ptr[3] = 0;
        break;
    case 2:
        argb_ptr[1] = 0;
        argb_ptr[2] = value;
        argb_ptr[3] = t;
        break;
    case 3:
        argb_ptr[1] = 0;
        argb_ptr[2] = q;
        argb_ptr[3] = value;
        break;
    case 4:
        argb_ptr[1] = t;
        argb_ptr[2] = 0;
        argb_ptr[3] = value;
        break;
    case 5:
    default:
        argb_ptr[1] = value;
        argb_ptr[2] = 0;
        argb_ptr[3] = q;
        break;
    }
}

OnlineProcessor::OnlineProcessor(QObject *parent) : QObject(parent),
    take_next_picture(false), is_recording(false), stimulus_started(false), timer_refresh_counter(0) {
    params = RecordParams::getParams();
    roi = RegionOfInterest::getRegion();
}

void OnlineProcessor::pushImage(const int computer_time, const ImageArray image_in) {
    if (!stimulus_started) {
        emit(yieldImage(drawBufferFromRaw(image_in)));
        return;
    }
    if (image_in.length() != roi->length()) {emit(raiseError("roi_in != roi")); return;}
    int timestamp = 0;
    if (camera_expose_computer_time.length() > 0)  { timestamp = int((computer_time - camera_expose_computer_time.last()) / params->getExposureTime()) * params->getExposureTime() + camera_expose_timestamp.last(); }
    double phase = (timestamp - diode_timestamp.last()) * phase_per_ms + pulse_id * phase_per_pulse - phase_exposure_offset;
    camera_phase << phase;
    processFrame(image_in, phase);
    if (timer_refresh_counter > 10) {  // because drawBufferFromDFT is very slow
        emit(yieldImage(drawBufferFromRaw(image_in)));
        emit(reportRemainingTime(int((end_time - timestamp)/1000)));
        timer_refresh_counter = 0;
    }
    timer_refresh_counter++;
    emit(yieldImageData(timestamp, image_in));
    if (timestamp > end_time) { stopRecording(); }
}

void OnlineProcessor::pushDiodeSignal(const int timestamp, const double signal) {
    if (!is_recording) { return; }  // we don't care about diode signal unless it's recording
    if (!stimulus_started) {  // recording doesn't start until the first diode signal
        stimulus_started = true;
        start_time = timestamp;
        end_time = start_time + params->getCycleNo() * params->getPeriodInSeconds() * 1000 + 3 * params->getExposureTime();
        pulse_id = 0;
    } else {
        pulse_id ++;
        if (pulse_id == pulse_per_cycle) { pulse_id = 0; }
    }
    diode_timestamp << timestamp;
    diode_signal << signal;
}

void OnlineProcessor::pushCameraSignal(const int timestamp, const int computer_time) {
    if (!stimulus_started) { return; }
    camera_expose_computer_time << computer_time;
    camera_expose_timestamp << timestamp;
}

QImage OnlineProcessor::drawBufferFromRaw(const ImageArray image_in) {
    QImage image(roi->getSize(), QImage::Format_Indexed8);
    QVector<QRgb> color_table;
    color_table << qRgb(0, 255, 0) << qRgb(0, 255, 0);
    for (int i = 2; i < 169; i++) { color_table << qRgb(i, i, i); }
    // the camera bleeds at ~ 180, so I set a guard here. when adjusting light keep the blue circle small.
    color_table << qRgb(0, 0, 255);
    for (int i = 170; i < 255; i++) { color_table << qRgb(i, i, i); }
    color_table << qRgb(255, 0, 0);
    image.setColorTable(color_table);
    const quint16 *const image_in_data = image_in.constData();
    int image_height = image.height();
    int image_width = image.width();
    for (int i = 0; i < image_height; i++) {
        uchar *line = image.scanLine(i);
        int line_start = i * image_width;
        for (int j = 0; j < image_width; j++)
            line[j] = image_in_data[line_start + j] >> 8;
    }
    if (take_next_picture) {
        qDebug() << "write on picture";
        QImageWriter file(file_path, "png");
        file.setQuality(1);
        // change the image back to no saturation coloring
        color_table[0] = qRgb(0, 0, 0);
        color_table[1] = qRgb(1, 1, 1);
        color_table[169] = qRgb(169, 169, 169);
        image.setColorTable(color_table);
        file.write(image);
        take_next_picture = false;
    }
    return image;
}

void OnlineProcessor::processFrame(const ImageArray image_in, const double phase) {
    double coef_real = qCos(phase);
    double coef_imag = qSin(phase);
    int length = image_in.length();
    for (int i = 0; i < length; i++) {
        quint16 pixel = image_in.at(i);
        imag[i] -= coef_imag * pixel;
        real[i] += coef_real * pixel;
        ave[i] += pixel;
    }
}

QImage OnlineProcessor::drawBufferFromDFT() {
    QImage image(roi->getSize(), QImage::Format_ARGB32);
    int length = real.length();
    double max_power = 0;
    DoubleArray power, angle;
    power.resize(real.size());
    angle.resize(real.size());
    for (int i = 0; i < length; i++) {
        power[i] = sqrt(real.at(i) * real.at(i));
        if (power.at(i) > max_power) max_power = power.at(i);
        angle[i] = (std::atan2(real.at(i), imag.at(i)) * M_1_PI + 1) * 180;
    }
    max_power = max_power / 255.0;
    int height = image.height();
    int width = image.width();
    for (int j = 0; j < height; j++) {
        uchar *image_data = image.scanLine(j);
        for (int i = 0; i < width; i++) {
            int location = j * width + i;
            polar2rgb(angle.at(location), uchar(power.at(location) / max_power), image_data + i * 4);
        }
    }
    return image;
}

void OnlineProcessor::startRecording() {
    if (is_recording) {emit(raiseError("attempt to start recording while OnlineProcessor is in recording mode")); return;}
    is_recording = true;
    roi->lock();
    real.resize(roi->length());
    imag.resize(roi->length());
    ave.resize(roi->length());
    // clear all the arrays
    real.fill(0);
    imag.fill(0);
    ave.fill(0);
    diode_signal.clear();
    diode_timestamp.clear();
    camera_expose_computer_time.clear();
    camera_expose_timestamp.clear();
    camera_phase.clear();
    // calculate coefficients for phase calculation
    phase_per_pulse = (M_PI * 2 * params->getFramePerPulse()) / params->getPeriodInFrames();
    phase_per_ms = (M_PI * 2) / (params->getPeriodInSeconds() * 1000);
    phase_exposure_offset = - (phase_per_ms * params->getExposureTime() / 2);
    pulse_per_cycle = params->getPeriodInFrames() / params->getFramePerPulse();
}

void OnlineProcessor::stopRecording() {
    if (!is_recording) {emit(raiseError("attempt to stop recording while OnlineProcessor is not in recording mode")); return;}
    is_recording = false;
    stimulus_started = false;
    emit(saveDoubleSeries(diode_signal, "diode_signal"));
    emit(saveIntSeries(diode_timestamp, "diode_nidaq_time"));
    emit(saveIntSeries(camera_expose_timestamp, "readout_nidaq_time"));
    emit(saveIntSeries(camera_expose_computer_time, "readout_computer_time"));
    emit(saveDoubleSeries(camera_phase, "cam_phase"));
    emit(saveDoubleSeries(real, "online_real"));
    emit(saveDoubleSeries(imag, "online_imag"));
    emit(saveDoubleSeries(ave, "online_ave"));
    emit(finishSaving());
    emit(reportRemainingTime(0));
}

void OnlineProcessor::takePicture(QString file_path_in) {
    file_path = file_path_in;
    take_next_picture = true;
}
