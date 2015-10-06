#ifndef SAVEWORKER_H
#define SAVEWORKER_H

#include <QObject>

class saveWorker : public QObject
{
    Q_OBJECT
public:
    explicit saveWorker(QObject *parent = 0);

signals:

public slots:
};

#endif // SAVEWORKER_H
