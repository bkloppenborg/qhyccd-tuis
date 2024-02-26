
// local includes
#include "cvfits.hpp"
#include "datetime_utilities.hpp"
#include "coordinate_conversions.hpp"

// system includes
#include <fitsio2.h>
#include <math.h>
#include <algorithm>
#include <opencv2/core.hpp>

CVFITS::CVFITS(std::string filename) {
  fitsfile * fptr;
  int status = 0;
  int nfound = 1;
  long naxes[3] = {0, 0, 0};
  int nullval = 0;
  int anynull = 0;

  // open the file
  fits_open_file(&fptr, filename.c_str(), READONLY, &status);

  // Get the dimensions of the image
  fits_read_keys_lng(fptr, "NAXIS", 1, 3, naxes, &nfound, &status);
  int width = naxes[0];
  int height = naxes[1];
  int depth = naxes[2];

  int nelements = width * height;

  // Read in the image
  if(naxes[2] == 1) {
    // single channel image
    this->image = cv::Mat(height, width, CV_16UC1);
    fits_read_img(fptr, TUSHORT, 1, nelements, &nullval, this->image.ptr(), &anynull, &status);

  } else {
    // multi-channel image
    std::vector<cv::Mat> channels;
    for(int i = 0; i < depth; i++)
      channels.push_back(cv::Mat(height, width, CV_16UC1));

    for(int i = 0; i < depth; i++) {
      long fpixel[3] = {1, 1, 1+i};
      fits_read_pix(fptr, TUSHORT, fpixel, nelements, &nullval, channels[i].ptr(), &anynull, &status);
    }

    cv::merge(channels, this->image);
  }
 }

void CVFITS::saveToFITS(std::string filename, bool overwrite) {

  fitsfile * fptr;
  int status = 0;

  long width = this->image.cols;
  long height = this->image.rows;
  long depth = this->image.channels();

  int bitpix = USHORT_IMG;
  long naxis = 2;

  if(depth == 3)
    naxis = 3;

  std::vector<long> naxes;
  naxes.push_back(width);
  naxes.push_back(height);

  if(naxis == 3)
    naxes.push_back(depth);

  int nelements = width * height;

  // open the file
  fits_create_file(&fptr, filename.c_str(), &status);

  if(depth > 1) {
    // split the image channels, write them out to the image independently.
    std::vector<cv::Mat> channels;
    cv::split(this->image, channels);

    // Write out each channel independently.
    // NOTE: OpenCV stores data in BGR order.
    fits_create_img(fptr, bitpix, naxis, naxes.data(), &status);
    for(int i = 0; i < depth; i++) {
      long fpixel[3] = {1, 1, 1+i};
      fits_write_pix(fptr, TUSHORT, fpixel, nelements, channels[i].ptr(), &status);
    }

  } else {
    // single channel image, write it out.
    fits_create_img(fptr, bitpix, naxis, naxes.data(), &status);
    fits_write_img(fptr, TUSHORT, 1, nelements, this->image.data, &status);
  }

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

  fits_write_key(fptr, TSTRING, "BINNING",
                 (void *) bin_mode_name.c_str(),
                 "Binning mode for the camera",
                 &status);

  fits_write_key(fptr, TUINT, "XBINNING",
                 (void *) &xbinning,
                 "Bnning factor used on X axis",
                 &status);

  fits_write_key(fptr, TUINT, "YBINNING",
                 (void *) &ybinning,
                 "Bnning factor used on Y axis",
                 &status);

  // Write color channel information for tri-color images.
  if(depth == 3) {
    fits_write_key(fptr, TSTRING, "CSPACE",  (void*) "RGB", "Colorspace of stored images", &status);
    fits_write_key(fptr, TSTRING, "CTYPE3",  (void*) "BAND-SET", "Type of color part in 4-3 notation", &status);
    fits_write_key(fptr, TSTRING, "CNAME3",  (void*) "Color-Space", "Description", &status);
    // NOTE: OpenCV stores images in BGR order.
    fits_write_key(fptr, TSTRING, "CSBAND1", (void*) "Blue", "Color Band for Channel 1", &status);
    fits_write_key(fptr, TSTRING, "CSBAND2", (void*) "Green", "Color Band for Channel 2", &status);
    fits_write_key(fptr, TSTRING, "CSBAND3", (void*) "Red", "Color Band for Channel 3", &status);
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
  fits_write_key(fptr, TDOUBLE, "EXPTIME",
                 (void*) &exposure_duration_sec,
                 "Duration of exposure in seconds",
                 &status);

  fits_write_key(fptr, TSTRING, "FILTER",
                 (void*) filter_name.c_str(),
                 "Name of photometric filter used",
                 &status);

  fits_write_key(fptr, TDOUBLE, "GAIN",
                 (void*) &gain,
                 "Camera Gain Setting",
                 &status);

  fits_write_key(fptr, TDOUBLE, "EGAIN",
                 (void*) &gain,
                 "Camera Gain Setting",
                 &status);


  //
  // Information about the object
  //
  fits_write_key(fptr, TSTRING, "CATALOG",
                 (void *) catalog_name.c_str(),
                 "Name of catalog to which the object belongs",
                 &status);

  // Replace any underscores in the name with spaces.
  std::replace(object_name.begin(), object_name.end(), '_', ' ');
  fits_write_key(fptr, TSTRING, "OBJECT",
                 (void *) object_name.c_str(),
                 "Name of object from the catalog.",
                 &status);

  //
  // Latitude, Longitude, and Altitude
  // Adopt the convention from the EOSSA File Specification v. 3.1.1
  //
  double t_latitude = latitude * 180.0 / M_PI;
  fits_write_key(fptr, TDOUBLE, "TELLONG",
                 (void *) &t_latitude,
                 "Latitude of observatory (degrees).",
                 &status);
  double t_longitude = longitude * 180.0 / M_PI;
  fits_write_key(fptr, TDOUBLE, "TELLAT",
                 (void *) &t_longitude,
                 "Longitude of observatory (degrees)",
                 &status);
  fits_write_key(fptr, TDOUBLE, "TELALT",
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