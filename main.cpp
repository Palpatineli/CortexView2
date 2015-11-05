/* ***********************************
 * CortexView 2 is a optical imaging software that is designed for continuous
 * imaging methods (Kalatsky & Stryker 2002).
 * It uses PVCam to drive the camera and currently written to use Photometric
 * casacade 512B.
 * Developed at the Sur lab, Picower Institue for Learning and Memory, M.I.T.
 * Author: Keji Li <mail@keji.li>
 */
#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    MainWindow w;
    w.show();

    return a.exec();
}
