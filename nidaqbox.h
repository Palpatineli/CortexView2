#ifndef NIDAQSIGNALSTREAM_H
#define NIDAQSIGNALSTREAM_H

#include <QObject>

class NIDaqSignalStream : public QObject
{
    Q_OBJECT
public:
    explicit NIDaqSignalStream(QObject *parent = 0);

signals:

public slots:
};

#endif // NIDAQSIGNALSTREAM_H
