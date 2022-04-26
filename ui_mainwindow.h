/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.5.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QWidget>
#include "cameraview.h"

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralWidget;
    QWidget *controlPanel;
    QPushButton *recordButton;
    QPushButton *resetROIButton;
    QPushButton *cameraButton;
    QSpinBox *cycleNoSpin;
    QLineEdit *filePathEdit;
    QSpinBox *periodSpinInF;
    QPushButton *browseFileButton;
    QDoubleSpinBox *periodSpinInS;
    QLabel *label;
    QLabel *label_2;
    QLabel *label_3;
    QPushButton *binningButton;
    QSpinBox *framePerPulse;
    QLabel *label_4;
    CameraView *cameraView;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QStringLiteral("MainWindow"));
        MainWindow->setEnabled(true);
        MainWindow->resize(512, 572);
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(MainWindow->sizePolicy().hasHeightForWidth());
        MainWindow->setSizePolicy(sizePolicy);
        QIcon icon;
        icon.addFile(QStringLiteral(":/new/icon/img/icon-cortex-view.png"), QSize(), QIcon::Normal, QIcon::Off);
        MainWindow->setWindowIcon(icon);
        centralWidget = new QWidget(MainWindow);
        centralWidget->setObjectName(QStringLiteral("centralWidget"));
        sizePolicy.setHeightForWidth(centralWidget->sizePolicy().hasHeightForWidth());
        centralWidget->setSizePolicy(sizePolicy);
        controlPanel = new QWidget(centralWidget);
        controlPanel->setObjectName(QStringLiteral("controlPanel"));
        controlPanel->setGeometry(QRect(0, 512, 512, 60));
        recordButton = new QPushButton(controlPanel);
        recordButton->setObjectName(QStringLiteral("recordButton"));
        recordButton->setEnabled(true);
        recordButton->setGeometry(QRect(10, 10, 40, 40));
        QIcon icon1;
        icon1.addFile(QStringLiteral(":/new/icon/img/icon-record-alt.png"), QSize(), QIcon::Normal, QIcon::Off);
        icon1.addFile(QStringLiteral(":/new/icon/img/icon-record-inactive.png"), QSize(), QIcon::Normal, QIcon::On);
        icon1.addFile(QStringLiteral(":/new/icon/img/icon-stop-inactive.png"), QSize(), QIcon::Active, QIcon::On);
        icon1.addFile(QStringLiteral(":/new/icon/img/icon-record-alt.png"), QSize(), QIcon::Selected, QIcon::On);
        recordButton->setIcon(icon1);
        recordButton->setIconSize(QSize(32, 32));
        recordButton->setCheckable(true);
        resetROIButton = new QPushButton(controlPanel);
        resetROIButton->setObjectName(QStringLiteral("resetROIButton"));
        resetROIButton->setGeometry(QRect(110, 10, 40, 40));
        QIcon icon2;
        icon2.addFile(QStringLiteral(":/new/icon/img/icon-reset-roi.png"), QSize(), QIcon::Normal, QIcon::On);
        resetROIButton->setIcon(icon2);
        resetROIButton->setIconSize(QSize(32, 32));
        cameraButton = new QPushButton(controlPanel);
        cameraButton->setObjectName(QStringLiteral("cameraButton"));
        cameraButton->setGeometry(QRect(60, 10, 40, 40));
        QIcon icon3;
        icon3.addFile(QStringLiteral(":/new/icon/img/icon-take-still.png"), QSize(), QIcon::Normal, QIcon::Off);
        cameraButton->setIcon(icon3);
        cameraButton->setIconSize(QSize(32, 32));
        cycleNoSpin = new QSpinBox(controlPanel);
        cycleNoSpin->setObjectName(QStringLiteral("cycleNoSpin"));
        cycleNoSpin->setGeometry(QRect(360, 34, 45, 18));
        QFont font;
        font.setPointSize(8);
        cycleNoSpin->setFont(font);
        cycleNoSpin->setButtonSymbols(QAbstractSpinBox::PlusMinus);
        cycleNoSpin->setAccelerated(true);
        cycleNoSpin->setMinimum(1);
        cycleNoSpin->setMaximum(80);
        cycleNoSpin->setSingleStep(5);
        cycleNoSpin->setValue(20);
        filePathEdit = new QLineEdit(controlPanel);
        filePathEdit->setObjectName(QStringLiteral("filePathEdit"));
        filePathEdit->setGeometry(QRect(210, 8, 279, 18));
        filePathEdit->setFont(font);
        periodSpinInF = new QSpinBox(controlPanel);
        periodSpinInF->setObjectName(QStringLiteral("periodSpinInF"));
        periodSpinInF->setGeometry(QRect(210, 34, 40, 18));
        periodSpinInF->setFont(font);
        periodSpinInF->setButtonSymbols(QAbstractSpinBox::PlusMinus);
        periodSpinInF->setAccelerated(true);
        periodSpinInF->setMinimum(6);
        periodSpinInF->setMaximum(2000);
        periodSpinInF->setSingleStep(6);
        periodSpinInF->setValue(900);
        browseFileButton = new QPushButton(controlPanel);
        browseFileButton->setObjectName(QStringLiteral("browseFileButton"));
        browseFileButton->setGeometry(QRect(487, 8, 15, 18));
        sizePolicy.setHeightForWidth(browseFileButton->sizePolicy().hasHeightForWidth());
        browseFileButton->setSizePolicy(sizePolicy);
        browseFileButton->setFont(font);
        periodSpinInS = new QDoubleSpinBox(controlPanel);
        periodSpinInS->setObjectName(QStringLiteral("periodSpinInS"));
        periodSpinInS->setGeometry(QRect(290, 34, 55, 18));
        periodSpinInS->setFont(font);
        periodSpinInS->setButtonSymbols(QAbstractSpinBox::PlusMinus);
        periodSpinInS->setAccelerated(true);
        periodSpinInS->setValue(12);
        label = new QLabel(controlPanel);
        label->setObjectName(QStringLiteral("label"));
        label->setGeometry(QRect(252, 34, 40, 18));
        label->setFont(font);
        label_2 = new QLabel(controlPanel);
        label_2->setObjectName(QStringLiteral("label_2"));
        label_2->setGeometry(QRect(347, 34, 10, 18));
        label_2->setFont(font);
        label_3 = new QLabel(controlPanel);
        label_3->setObjectName(QStringLiteral("label_3"));
        label_3->setGeometry(QRect(407, 34, 36, 18));
        label_3->setFont(font);
        binningButton = new QPushButton(controlPanel);
        binningButton->setObjectName(QStringLiteral("binningButton"));
        binningButton->setEnabled(true);
        binningButton->setGeometry(QRect(160, 10, 40, 40));
        QIcon icon4;
        icon4.addFile(QStringLiteral(":/new/icon/img/icon-binning-inactive.png"), QSize(), QIcon::Normal, QIcon::Off);
        icon4.addFile(QStringLiteral(":/new/icon/img/icon-binning-inactive.png"), QSize(), QIcon::Active, QIcon::Off);
        icon4.addFile(QStringLiteral(":/new/icon/img/icon-binning-active.png"), QSize(), QIcon::Active, QIcon::On);
        icon4.addFile(QStringLiteral(":/new/icon/img/icon-binning-inactive.png"), QSize(), QIcon::Selected, QIcon::Off);
        icon4.addFile(QStringLiteral(":/new/icon/img/icon-binning-inactive.png"), QSize(), QIcon::Selected, QIcon::On);
        binningButton->setIcon(icon4);
        binningButton->setIconSize(QSize(32, 32));
        binningButton->setCheckable(true);
        binningButton->setChecked(true);
        framePerPulse = new QSpinBox(controlPanel);
        framePerPulse->setObjectName(QStringLiteral("framePerPulse"));
        framePerPulse->setGeometry(QRect(440, 34, 30, 18));
        framePerPulse->setMinimum(1);
        framePerPulse->setMaximum(12);
        framePerPulse->setValue(6);
        label_4 = new QLabel(controlPanel);
        label_4->setObjectName(QStringLiteral("label_4"));
        label_4->setGeometry(QRect(472, 34, 47, 18));
        cameraView = new CameraView(centralWidget);
        cameraView->setObjectName(QStringLiteral("cameraView"));
        cameraView->setEnabled(true);
        cameraView->setGeometry(QRect(0, 0, 512, 512));
        sizePolicy.setHeightForWidth(cameraView->sizePolicy().hasHeightForWidth());
        cameraView->setSizePolicy(sizePolicy);
        cameraView->setMouseTracking(true);
        MainWindow->setCentralWidget(centralWidget);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "CortexView 2", 0));
#ifndef QT_NO_TOOLTIP
        recordButton->setToolTip(QApplication::translate("MainWindow", "Start/End recording session", 0));
#endif // QT_NO_TOOLTIP
        recordButton->setText(QString());
#ifndef QT_NO_TOOLTIP
        resetROIButton->setToolTip(QApplication::translate("MainWindow", "Reset ROI to full screen", 0));
#endif // QT_NO_TOOLTIP
        resetROIButton->setText(QString());
#ifndef QT_NO_TOOLTIP
        cameraButton->setToolTip(QApplication::translate("MainWindow", "Take still image", 0));
#endif // QT_NO_TOOLTIP
        cameraButton->setText(QString());
        browseFileButton->setText(QApplication::translate("MainWindow", "...", 0));
        label->setText(QApplication::translate("MainWindow", "frames", 0));
        label_2->setText(QApplication::translate("MainWindow", "s", 0));
        label_3->setText(QApplication::translate("MainWindow", "cycles", 0));
#ifndef QT_NO_TOOLTIP
        binningButton->setToolTip(QApplication::translate("MainWindow", "toggle 2x2 binning or no binning", 0));
#endif // QT_NO_TOOLTIP
        binningButton->setText(QString());
        label_4->setText(QApplication::translate("MainWindow", "pulse", 0));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
