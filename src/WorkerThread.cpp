#include <QThread>
#include <QString>
#include <QVariant>

#include "WorkerThread.hpp"
#include "camera_control.hpp"

void WorkerThread::setConfig(const QMap<QString, QVariant> & config) {
    mConfig = config;
}

void WorkerThread::run() {
    takeExposures(mConfig);
}


