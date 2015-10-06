#ifndef SESSIONDIALOG_H
#define SESSIONDIALOG_H

#include <QDialog>
#include "recordparams.h"

class RecordParams;

namespace Ui {
class SessionDialog;
}

class SessionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SessionDialog(const RecordParams& params_in, QWidget *parent = 0);
    ~SessionDialog();

public slots:
    void verify();
    void onBrowse();

public:
    void getRecordingParams();

private:
    Ui::SessionDialog *ui;
    RecordParams* params_ptr;
};

#endif // SESSIONDIALOG_H
