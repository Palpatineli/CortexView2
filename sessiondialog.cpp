#include <QDir>
#include <QMessageBox>
#include <QFileDialog>
#include <QSettings>

#include "recordparams.h"
#include "sessiondialog.h"
#include "ui_sessiondialog.h"

SessionDialog::SessionDialog(const RecordParams* params_in, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SessionDialog)
{
    params_ptr = params_in;
    ui->filePathEdit->setText(params_in.file_path);
    ui->lengthSpin->setValue(params_in.cycle_no);
    ui->periodSpin->setValue(params_in.period_in_frames);
    ui->periodSpinInS->setValue(params_in.period_in_seconds);

    connect(ui->browseFileButton, SIGNAL(released()), this, SLOT(onBrowse()));
    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(verify()));
    connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    ui->setupUi(this);
}

SessionDialog::~SessionDialog()
{
    delete ui;
}

void SessionDialog::getRecordingParams() {
    params_ptr->period_in_frames = ui->periodSpin->value();
    params_ptr->period_in_seconds = ui->periodSpinInS->value();
    params_ptr->cycle_no = ui->lengthSpin->value();
    params_ptr->file_path = ui->filePathEdit->text();
}

void SessionDialog::verify() {
    QDir file_path(ui->filePathEdit->text());
    if (file_path.cdUp()) {
        accept();
        return;
    }
    QMessageBox::warning(this, tr("Directory does not Exit!"), tr("Please re-enter your save file folder."), QMessageBox::Close);
    reject();
}

void SessionDialog::onBrowse() {
    QString file_path = QFileDialog::getSaveFileName(this, tr("Open Image"), ui->filePathEdit->text(), tr("Data Files (*.h5)"));
    ui->filePathEdit->setText(file_path);
}
