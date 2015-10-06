#include <qmath.h>
#include "onlineprocessor.h"

OnlineProcessor::OnlineProcessor(QObject *parent) : QObject(parent), phase(-1) {
    // initialize draw_buffer to make non-transparent
    for (int i = 0; i < 512; i ++) {
        for (int j = 0; j < 2048; j += 4) draw_buffer[i] = 255;
    }
}

void OnlineProcessor::updateImage(quint64 /*timestamp*/, double phase_in, const quint16 *image_in[512][512]) {
    buffer = image_in;
    phase = phase_in;
    if (!is_recording) {
        drawBufferFromRaw();
    } else {
        processFrame();
        drawBufferFromRaw();
    }
    emit(yieldImage(&draw_buffer, roi));
}

void OnlineProcessor::drawBufferFromRaw() {
    size_t x_end = roi.right();
    size_t y_end = roi.bottom();
    size_t location;
    for (size_t k = image_rect.x(); k < x_end; k++) {
        for (size_t l = image_rect.y(); l < y_end; l++) {
            location = k * 512 + l;
            pixel = buffer[location] >> 8;
            location = location * 4;
            if (pixel == 255) {  // QImage::Format_ARGB32
                draw_buffer[location + 1] = 255;  // R
                draw_buffer[location + 2] = 0;  // G
                draw_buffer[location + 3] = 0;  // B
            } else if (pixel == 0) {
                draw_buffer[location + 1] = 0;
                draw_buffer[location + 2] = 255;
                draw_buffer[location + 3] = 0;
            } else {
                draw_buffer[location + 1] = pixel;
                draw_buffer[location + 2] = pixel;
                draw_buffer[location + 3] = pixel;
            }
        }
    }
}

void OnlineProcessor::processFrame() {
    if (phase > - 0.001) {  // unless phase is a valid value
        qreal coef_real = qcos(phase);
        qreal coef_imag = qsin(phase);
        int x_end = roi.right();
        for (int i = roi.top(); i < roi.bottom(); i++) {
            for (int j = roi.left(); j < x_end; j++) {
                location = i * 512 + j;
                onlineAddition_imag[location] -= coef_imag * buffer[location];
                onlineAddition_real[location] += coef_real * buffer[location];
            }
        }
    }
    image_no++;
}

void OnlineProcessor::startRecording(const QRect &roi_in) {
    roi = roi_in;
    is_recording = true;
    // clear the slate
    for (int i = 0; i < 512 * 512; i++) {
            onlineAddition_real[i] = 0;
            onlineAddition_imag[i] = 0;
    }
    image_no = 0;
}

void OnlineProcessor::takePicture() {
    quint16 new_buffer = new quint16[512][512];
    memcpy(new_buffer, buffer, 512*512* sizeof(quint16));
    emit(yieldRawImage(new_buffer));
}

void OnlineProcessor::stopRecording() {
    emit(yieldDFT(onlineAddition_real, onlineAddition_imag));
    is_recording = false;
    phase = -1;
}
