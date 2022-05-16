#include "livewindow.h"

inline void initResource() { Q_INIT_RESOURCE(livewindow); }

QString LiveWindow::loadLiveWindow()
{
    initResource();
    return QStringLiteral("qrc:/com/machinekoder/live/LiveWindow.qml");
}
