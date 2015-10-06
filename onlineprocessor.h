#ifndef ONLINEPROCESSOR_H
#define ONLINEPROCESSOR_H

#include <QObject>

class OnlineProcessor : public QObject
{
    Q_OBJECT
public:
    explicit OnlineProcessor(QObject *parent = 0);

signals:

public slots:
};

#endif // ONLINEPROCESSOR_H
