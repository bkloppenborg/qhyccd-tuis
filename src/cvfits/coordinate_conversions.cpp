// system includes
#include <cmath>
#include <ctgmath>
#include <iomanip>
#include <sstream>
#include <string>
#include <iostream>

// project includes

// local includes
#include "coordinate_conversions.hpp"

namespace CoordinateConversion {

  void RadToDMS(double value, int &degrees, int &minutes, int &seconds) {
    // convert from radians to degrees
    value = value * 180.0 / M_PI;
    int sign = 1;
    if(value < 0) {
      sign = -1;
      value = fabs(value);
    }

    // extract the degrees, minutes, and seconds from the value
    double d, m, s;
    value = modf(value, &d);
    value = modf(value * 60, &m);
    value = modf(value * 60, &s);

    // set output values
    degrees = int(d) * sign;
    minutes = int(m);
    seconds = int(s);
  }

  double DMSToRad(int degrees, int minutes, int seconds) {
    double value = degrees + double(minutes) / 60 + double(seconds) / 3600;
    return value * M_PI / 180.0;
  }

  void RadToHMS(double value, int &hours, int &minutes, int & seconds) {
    // convert value to hours
    value = value * 12.0 / M_PI;

    int sign = 1;
    if (value < 0) {
      sign = -1;
      value = fabs(value);
    }

    // extract the degrees, minutes, and seconds from the value
    double h, m, s;
    value = modf(value, &h);
    value = modf(value * 60, &m);
    value = modf(value * 60, &s);

    // set output values
    hours   = int(h) * sign;
    minutes = int(m);
    seconds = int(s);
  }

  std::string RadToDMS(double value) {
    using namespace std;

    int degrees, minutes, seconds;
    RadToDMS(value, degrees, minutes, seconds);

    stringstream ss;
    ss << setw(4) << setfill('0') << internal << showpos << degrees << "d "
       << setw(2) << setfill('0') << noshowpos << minutes << "m "
       << setw(2) << setfill('0') << noshowpos << seconds << "s";

    return ss.str();
  }

  std::string RadToHMS(double value) {
    using namespace std;

    int hours, minutes, seconds;
    RadToHMS(value, hours, minutes, seconds);

    stringstream ss;
    ss << setw(2) << setfill('0') << internal << showpos << hours << "h "
       << setw(2) << setfill('0') << noshowpos << minutes << "m "
       << setw(2) << setfill('0') << noshowpos << seconds << "s";

    return ss.str();
  }

  double AsecToRad(double value) { return value / 206265; }

  double RadToAsec(double value) { return value * 206265; }

  //
}; // namespace CoordinateConversion
