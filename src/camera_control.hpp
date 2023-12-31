#ifndef CAMERA_CONTROL_H
#define CAMERA_CONTROL_H

#include <QString>
#include <QVariant>

int takeExposures(const QMap<QString, QVariant> & config);

#endif // CAMERA_CONTROL_H