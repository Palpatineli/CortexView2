#include <qmath.h>
#include <QColor>
#include <QDebug>
#include <QImageWriter>
#include "onlineprocessor.h"

OnlineProcessor::OnlineProcessor(QObject *parent) : QObject(parent),
    take_next_picture(false), is_recording(false), frame_no(0) {
}

void OnlineProcessor::updateImage(const quint64 /*timestamp*/, const double phase_in, const ImageArray image_in, const QRect roi_in) {
    if (is_recording && (phase_in > -0.01)) { // phase is -1 before NiDaqbox Starts receiving diode signals
        if (roi_in != roi) {emit(raiseError("roi_in != roi")); return;}
        //processFrame(image_in, phase_in);
        //emit(yieldImage(drawBufferFromDFT(), roi));
        emit(yieldImage(drawBufferFromRaw(image_in), roi));
    } else {
        roi = roi_in;
        emit(yieldImage(drawBufferFromRaw(image_in), roi));
    }
}

QImage OnlineProcessor::drawBufferFromRaw(const ImageArray image_in) {
    QImage image(roi.width(), roi.height(), QImage::Format_Indexed8);
    QVector<QRgb> color_table;
    color_table << qRgb(0, 255, 0);
    for (int i = 1; i < 255; i++) color_table << qRgb(i, i, i);
    color_table << qRgb(255, 0, 0);
    image.setColorTable(color_table);

    int width = roi.width();
    for (int i = 0; i < roi.height(); i++) {
        uchar *line = image.scanLine(i);
        for (int j = 0; j < width; j++) {
            line[j] = image_in.at(i * width + j) >> 8;
        }
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

void OnlineProcessor::processFrame(const ImageArray image_in, qreal phase) {
    qreal coef_real = qCos(phase);
    qreal coef_imag = qSin(phase);
    quint64 length = image_in.length();
    for (quint64 i = 0; i < length; i++) {
        imag[i] -= coef_imag * image_in.at(i);
        real[i] += coef_real * image_in.at(i);
    }
}

QImage OnlineProcessor::drawBufferFromDFT() {
    QImage image(roi.width(), roi.height(), QImage::Format_ARGB32);
    DFTArray power_buffer;
    int length = real.length();
    power_buffer.resize(length);
    for (int i=0; i< length; i++) {
        qreal x, y; x = real.at(i); y = imag.at(i);
        power_buffer[i] = qSqrt(x * x + y * y);
    }
    qreal max_power = *std::max_element(power_buffer.constBegin(), power_buffer.constEnd());
    int width = roi.width();
    for (int i=0; i<roi.height(); i++) {
        for (int j=0; j<width; j++) {
            int location = i * width + j;
            QColor pixel;
            pixel.fromHsvF(qAtan(imag.at(location) / real.at(location)) * 0.5 * M_1_PI, power_buffer.at(location) / max_power, 1);
            image.setPixel(j, i, pixel.rgba());
        }
    }
    return image;
}

void OnlineProcessor::startRecording() {
    if (is_recording) {emit(raiseError("attempt to start recording while OnlineProcessor is in recording mode")); return;}
    is_recording = true;
    real.resize(roi.width() * roi.height());
    real.clear();
    imag.resize(roi.width() * roi.height());
    imag.clear();
}

void OnlineProcessor::stopRecording() {
    if (!is_recording) {emit(raiseError("attempt to stop recording while OnlineProcessor is not in recording mode")); return;}
    is_recording = false;
}

void OnlineProcessor::takePicture(QString file_path_in) {
    file_path = file_path_in;
    qDebug() << "setting write on picture";
    take_next_picture = true;
}
