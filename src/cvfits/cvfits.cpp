
// local includes
#include "cvfits.hpp"
#include "datetime_utilities.hpp"
#include "coordinate_conversions.hpp"

// system includes
#include <fitsio2.h>
#include <math.h>


void CVFITS::saveToFITS(std::string filename, bool overwrite) {

  fitsfile * fptr;
  int status = 0;

  long width = this->image.cols;
  long height = this->image.rows;
  long depth = this->image.channels();


  int bitpix = USHORT_IMG;
  long naxis = 3;
  long naxes[3] = { (long int) width, (long int) height, (long int) depth };

  int nelements = width * height * depth;

  // open the file
  fits_create_file(&fptr, filename.c_str(), &status);

  // write the ImageData
  fits_create_img(fptr, bitpix, naxis, naxes, &status);
  fits_write_img(fptr, TUSHORT, 1, nelements, image.ptr(), &status);

  //
  // Information about the detector
  //
  fits_write_key(fptr, TSTRING, "DETNAME",
                 (void *) detector_name.c_str(),
                 "Name of detector used to make the observation",
                 &status);

  fits_write_key(fptr, TDOUBLE, "TEMP",
                 (void *) &temperature,
                 "Temperature of sensor in Celsius",
                 &status);

  // Write color channel information for tri-color images.
  if(depth == 3) {
    fits_write_key(fptr, TSTRING, "CSPACE",  (void*) "RGB", "Colorspace of stored images", &status);
    fits_write_key(fptr, TSTRING, "CTYPE3",  (void*) "BAND-SET", "Type of color part in 4-3 notation", &status);
    fits_write_key(fptr, TSTRING, "CNAME3",  (void*) "Color-Space", "Description", &status);
    fits_write_key(fptr, TSTRING, "CSBAND1", (void*) "Blue", "Color Band for Channel 1", &status);
    fits_write_key(fptr, TSTRING, "CSBAND1", (void*) "Green", "Color Band for Channel 1", &status);
    fits_write_key(fptr, TSTRING, "CSBAND1", (void*) "Red", "Color Band for Channel 1", &status);
  }

  //
  // Exposure settings.
  //
  std::string t_start = to_iso_8601(exposure_start);
  fits_write_key(fptr, TSTRING, "DATE-OBS", (void *)t_start.c_str(),
                 "ISO-8601 date-time for start exposure", &status);
                 
  fits_write_key(fptr, TSTRING, "DATE-BEG",
                 (void*)t_start.c_str(),
                 "ISO-8601 date-time for start exposure", &status);

  std::string t_end = to_iso_8601(exposure_end);
  fits_write_key(fptr, TSTRING, "DATE-END",
                 (void *)t_end.c_str(),
                 "ISO-8601 date-time for end exposure",
                 &status);

  std::string t = std::to_string(exposure_duration_sec);
  fits_write_key(fptr, TSTRING, "EXPTIME",
                 (void*) t.c_str(),
                 "Duration of exposure in seconds",
                 &status);

  fits_write_key(fptr, TSTRING, "FILTER",
                 (void*) filter_name.c_str(),
                 "Name of photometric filter used",
                 &status);


  //
  // Information about the object
  //
  fits_write_key(fptr, TSTRING, "CATALOG",
                 (void *) catalog_name.c_str(),
                 "Name of catalog to which the object belongs",
                 &status);

  fits_write_key(fptr, TSTRING, "OBJECT",
                 (void *) object_name.c_str(),
                 "Name of object from the catalog.",
                 &status);

  //
  // Latitude, Longitude, and Altitude
  //
  double t_latitude = latitude * 180.0 / M_PI;
  fits_write_key(fptr, TDOUBLE, "LATITUDE",
                 (void *) &t_latitude,
                 "Latitude of observatory (degrees).",
                 &status);
  double t_longitude = longitude * 180.0 / M_PI;
  fits_write_key(fptr, TDOUBLE, "LONGITUDE",
                 (void *) &t_longitude,
                 "Longitude of observatory (degrees)",
                 &status);
  fits_write_key(fptr, TDOUBLE, "ALTITUDE",
                 (void *) &altitude,
                 "Altitude of observatory (meters)",
                 &status);

  //
  // Image coordinate information.
  //
  if(ra_dec_set) {
    std::string ra_str  = CoordinateConversion::RadToHMS(ra);
    std::string dec_str = CoordinateConversion::RadToDMS(dec);
    fits_write_key(fptr, TSTRING, "RA",
                   (void *) ra_str.c_str(),
                   "Approximate RA of image center (HH:MM:SS.zzz)",
                   &status);

    fits_write_key(fptr, TSTRING, "DEC",
                   (void *) dec_str.c_str(),
                   "Approximate DEC of image center (DD:MM:SS.zzz)",
                   &status);
  } else if (azm_alt_set) {

    azm *= 180 / M_PI;
    alt *= 180 / M_PI;
    fits_write_key(fptr, TDOUBLE, "AZM",
                   (void *) &azm,
                   "Approximate AZM of image center (deg)",
                   &status);

    fits_write_key(fptr, TDOUBLE, "ALT",
                   (void *) &alt,
                   "Approximate ALT of image center (deg)",
                   &status);
  }

  // close the file
  fits_close_file(fptr, &status);
}