#include <qmath.h>
#include <cmath>
#include <QColor>
#include <QDebug>
#include <QImageWriter>
#include <QSettings>
#include "recordparams.h"
#include "regionofinterest.h"
#include "onlineprocessor.h"

OnlineProcessor::OnlineProcessor(QObject *parent) : QObject(parent),
    take_next_picture(false), is_recording(false), stimulus_started(false), slow_dft_counter(0) {
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
    processFrame(image_in, phase);
    if (slow_dft_counter > 10) {  // because drawBufferFromDFT is very slow
        emit(yieldImage(drawBufferFromDFT()));
        emit(reportRemainingTime(int((end_time - timestamp)/1000)));
        slow_dft_counter = 0;
    }
    slow_dft_counter++;
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
    color_table << qRgb(0, 255, 0);
    for (int i = 1; i < 255; i++) { color_table << qRgb(i, i, i); }
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
        imag[i] -= coef_imag * image_in.at(i);
        real[i] += coef_real * image_in.at(i);
    }
}

QImage OnlineProcessor::drawBufferFromDFT() {
    QImage image(roi->getSize(), QImage::Format_ARGB32);
    int length = real.length();
    std::pair<const double *, const double *> minmax_real = std::minmax_element(real.constBegin(), real.constEnd());
    std::pair<const double *, const double *> minmax_imag = std::minmax_element(imag.constBegin(), imag.constEnd());
    double max_real = std::max<double>(std::abs(*(minmax_real.first)), std::abs(*(minmax_real.second)));
    double max_imag = std::max<double>(std::abs(*(minmax_imag.first)), std::abs(*(minmax_imag.second)));
    double normalizer = std::max<double>(max_real / qSqrt(3), max_imag) / 255.0;
    uchar *image_data = image.bits();
    for (int i = 0; i < length; i++) { complex2rgb(real.at(i) / normalizer, imag.at(i) / normalizer, image_data + i * 4); }
    return image;
}

void OnlineProcessor::startRecording() {
    if (is_recording) {emit(raiseError("attempt to start recording while OnlineProcessor is in recording mode")); return;}
    is_recording = true;
    roi->lock();
    real.resize(roi->length());
    imag.resize(roi->length());
    // clear all the arrays
    real.fill(0);
    imag.fill(0);
    diode_signal.clear();
    diode_timestamp.clear();
    camera_expose_computer_time.clear();
    camera_expose_timestamp.clear();
    // calculate coefficients for phase calculation
    phase_per_pulse = (M_PI * 2 * frames_per_pulse) / params->getPeriodInFrames();
    phase_per_ms = (M_PI * 2) / (params->getPeriodInSeconds() * 1000);
    phase_exposure_offset = - (phase_per_ms * params->getExposureTime() / 2);
    pulse_per_cycle = params->getCycleNo() / frames_per_pulse;
}

void OnlineProcessor::stopRecording() {
    if (!is_recording) {emit(raiseError("attempt to stop recording while OnlineProcessor is not in recording mode")); return;}
    is_recording = false;
    stimulus_started = false;
    emit(saveDoubleSeries(diode_signal, "diode_signal"));
    emit(saveIntSeries(diode_timestamp, "diode_nidaq_time"));
    emit(saveIntSeries(camera_expose_timestamp, "readout_nidaq_time"));
    emit(saveIntSeries(camera_expose_computer_time, "readout_computer_time"));
    emit(finishSaving());
    emit(reportRemainingTime(0));
}

void OnlineProcessor::takePicture(QString file_path_in) {
    file_path = file_path_in;
    take_next_picture = true;
}

void OnlineProcessor::complex2rgb(double real_element, double imag_element, uchar *argb_ptr) {
    static const double tan30 = 1.0f / qSqrt(3);
    argb_ptr[0] = 0xff;
    double real_projection = tan30 * real_element;
    if (real_element > 0) {
        if (imag_element < 0 && imag_element < -real_projection) {
            argb_ptr[1] = quint8(-real_projection - imag_element);
            argb_ptr[2] = 0;
            argb_ptr[3] = quint8(real_projection - imag_element);
        } else {
            argb_ptr[1] = 0;
            argb_ptr[2] = quint8(real_projection + imag_element);
            argb_ptr[3] = quint8(real_projection * 2);
        }
    } else {
        if (imag_element < 0 && imag_element < real_projection) {
            argb_ptr[1] = quint8(-real_projection - imag_element);
            argb_ptr[2] = 0;
            argb_ptr[3] = quint8(real_projection - imag_element);
        } else {
            argb_ptr[1] = quint8(-2 * real_projection);
            argb_ptr[2] = quint8(imag_element - real_projection);
            argb_ptr[3] = 0;
        }
    }
}
