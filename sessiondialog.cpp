#include "sessiondialog.h"
#include "ui_sessiondialog.h"

sessionDialog::sessionDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::sessionDialog)
{
    ui->setupUi(this);
}

sessionDialog::~sessionDialog()
{
    delete ui;
}
