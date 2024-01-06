#ifndef SCALE_IMAGE_H
#define SCALE_IMAGE_H

#include <opencv2/core/mat.hpp>

/// @brief Applies a linear scale to a CV_16UC1 image using the minimum, maximum, median, and standard deviation.
/// @param rawImage the input i mage
/// @return a cv::Mat in CV_32FC1 format.
cv::Mat scaleImageLinear_CV_16UC1(const cv::Mat & rawImage);

/// @brief Applies a linear scale to a CV_16UC3 image by wrapping calls to scaleImageLinear_CV_16UC1
/// @param rawImage the input i mage
/// @return a cv::Mat in CV_32FC3 format.
cv::Mat scaleImageLinear_CV_16UC3(const cv::Mat & rawImage);

/// @brief Scales a single channel or multi-channel image of type CV_16UC1 or CV_16UC3
/// @param rawImage The input raw image.
/// @return A scaled image of type CV_8UC1 or CV_8UC3
cv::Mat scaleImageLinear(const cv::Mat & rawImage);

#endif // SCALE_IMAGE_H