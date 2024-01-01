#ifndef CAMERA_CONTROL_H
#define CAMERA_CONTROL_H

#include <QString>
#include <QVariant>

#include <qhyccd.h>


/// @brief Instructs th camera to take an exposure
/// @param config The requested camera and exposure configuration as generated by cli_parser
/// @return 0 on success, otherwise on failure.
int takeExposures(const QMap<QString, QVariant> & config);

/// @brief Sets the camera's binning mode from a string like "1x1"
/// @param handle Handle to the QHY Camera
/// @param binModeName The requested bin mode.
/// @param setMode Returns the bin mode that was actually set.
/// @param binX Returns the x-scale of the bin mode that was actually set.
/// @param binY Returns the y-scale of the bin mode that was actually set.
/// \return QHYCCD_SUCCESS on success, -1 otherwise.
int setCameraBinMode(qhyccd_handle * handle, const QString & requestedMode, QString & setMode, int & binX, int & binY);

void setTemperature(qhyccd_handle * handle, double setPointC);

void monitorTemperature(qhyccd_handle * handle);

int runCooler(const QMap<QString, QVariant> & config);

#endif // CAMERA_CONTROL_H