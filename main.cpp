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

#ifdef _MSC_VER
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
// Code to display the memory leak report
// We use a custom report hook to filter out Qt's own memory leaks
// Credit to Andreas Schmidts - http://www.schmidt-web-berlin.de/winfig/blog/?p=154

_CRT_REPORT_HOOK prevHook;
#define TRUE 1
#define FALSE 0

int customReportHook(int /* reportType */, char* message, int* /* returnValue */) {
    // This function is called several times for each memory leak.
    // Each time a part of the error message is supplied.
    // This holds number of subsequent detail messages after
    // a leak was reported
    const int numFollowupDebugMsgParts = 2;
    static bool ignoreMessage = false;
    static int debugMsgPartsCount = 0;

    // check if the memory leak reporting starts
    if ((strncmp(message, "Detected memory leaks!\n", 10) == 0)
        || ignoreMessage)
    {
        // check if the memory leak reporting ends
        if (strncmp(message, "Object dump complete.\n", 10) == 0)
        {
            _CrtSetReportHook(prevHook);
            ignoreMessage = FALSE;
        }
        else
            ignoreMessage = TRUE;

        // something from our own code?
        if (strstr(message, ".cpp") == NULL)
        {
            if (debugMsgPartsCount++ < numFollowupDebugMsgParts)
                // give it back to _CrtDbgReport() to be printed to the console
                return FALSE;
            else
                return TRUE;  // ignore it
        }
        else
        {
            debugMsgPartsCount = 0;
            // give it back to _CrtDbgReport() to be printed to the console
            return FALSE;
        }
    }
    else
        // give it back to _CrtDbgReport() to be printed to the console
        return FALSE;
}
#endif // _MSC_VER

int main(int argc, char *argv[])
{
#if defined(_MSC_VER)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
    prevHook = _CrtSetReportHook(customReportHook);
    // _CrtSetBreakAlloc(157); // Use this line to break at the nth memory allocation
#endif
    QApplication a(argc, argv);

    MainWindow w;
    w.show();

    int app_error = a.exec();

#if defined(_MSC_VER)
    // Once the app has finished running and has been deleted,
    // we run this command to view the memory leaks:
    printf("is there memory leaks: %d\n", _CrtDumpMemoryLeaks());
#endif

    return app_error;
}
