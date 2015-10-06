#ifndef SESSIONDIALOG_H
#define SESSIONDIALOG_H

#include <QDialog>

namespace Ui {
class sessionDialog;
}

class sessionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit sessionDialog(QWidget *parent = 0);
    ~sessionDialog();

private:
    Ui::sessionDialog *ui;
};

#endif // SESSIONDIALOG_H
