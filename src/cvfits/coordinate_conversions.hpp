#ifndef COORDINATE_CONVERSION_H
#define COORDINATE_CONVERSION_H

// system includes
#include <string>

// project includes

// local includes

namespace CoordinateConversion {

  void RadToDMS(double value, int &degrees, int &minutes, int &seconds);

  double DMSToRad(int degrees, int minutes, int seconds);

  void RadToHMS(double value, int &hours, int &minutes, int &seconds);

  double AsecToRad(double value);

  double RadToAsec(double value);

  std::string RadToDMS(double value);

  std::string RadToHMS(double value);

  //
}; // namespace CoordinateConversion

#endif // COORDINATE_CONVERSION_H
