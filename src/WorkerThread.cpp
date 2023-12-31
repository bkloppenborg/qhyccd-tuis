#include <QThread>
#include <QString>
#include <QVariant>

#include "WorkerThread.hpp"
#include "camera_control.hpp"

void WorkerThread::setConfig(const QMap<QString, QVariant> & config) {
    mConfig = config;
}

void WorkerThread::run() {

    bool cool_down = (mConfig["camera-cool-down"].toString() == "1");
    bool warm_up   = (mConfig["camera-warm-up"].toString() == "1");

    if(cool_down || warm_up) {
        runCooler(mConfig);
    } else {
        takeExposures(mConfig);
    }
}


