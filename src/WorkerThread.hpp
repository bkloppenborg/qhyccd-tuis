#ifndef WORKER_THREAD_H
#define WORKER_THREAD_H

#include <QThread>
#include <QString>
#include <QVariant>

#include "camera_control.hpp"

class WorkerThread : public QThread {
    Q_OBJECT

    QMap<QString, QVariant> mConfig;

public:
    void setConfig(const QMap<QString, QVariant> & config);

    void run() override;
};

#endif // WORKER_THREAD_H

