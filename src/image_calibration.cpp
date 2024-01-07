#include <opencv2/opencv.hpp>

#include "image_calibration.hpp"

cv::Mat scaleImageLinear_CV_16UC1(const cv::Mat & rawImage) {

    cv::Mat scaledImage;
    rawImage.convertTo(scaledImage, CV_32FC1, 1.0, 0.0);

    // Single channel image
    double min = 0;
    double max = 65535;
    double mean = 1;
    double stddev = 1;
    cv::Scalar mean_s;
    cv::Scalar stddev_s;

    cv::minMaxLoc(scaledImage, &min, &max);
    cv::meanStdDev(scaledImage, mean_s, stddev_s);

    double minPixValue = mean_s[0] - stddev_s[0];
    double maxPixValue = max;

    double scale = 255.0 / (double)(maxPixValue - minPixValue);

    cv::subtract(scaledImage, min, scaledImage);
    cv::multiply(scaledImage, scale, scaledImage);

    return scaledImage;
}

cv::Mat scaleImageLinear_CV_16UC3(const cv::Mat & rawImage) {

    int numChannels = rawImage.channels();

    // Split the channels and process them independently.
    std::vector<cv::Mat> channels;
    cv::split(rawImage, channels);
    for(int i = 0; i < numChannels; i++) {
        channels[i] = scaleImageLinear_CV_16UC1(channels[i]);
    }

    // Combine the channels.
    cv::Mat scaledImage;
    cv::merge(channels, scaledImage);

    return scaledImage;
}

/// @brief Scales a single channel or multi-channel image of type CV_16UC1 or CV_16UC3
/// @param rawImage The input raw image.
/// @return A scaled image of type CV_8UC1 or CV_8UC3
cv::Mat scaleImageLinear(const cv::Mat & rawImage) {

    cv::Mat scaledImage;
    cv::Mat outputArray;

    if(rawImage.channels() > 0) {
        scaledImage = scaleImageLinear_CV_16UC3(rawImage);
        scaledImage.convertTo(outputArray, CV_8UC3, 1.0, 0.0);
    } else {
        scaledImage = scaleImageLinear_CV_16UC1(rawImage);
        scaledImage.convertTo(outputArray, CV_8UC1, 1.0, 0.0);
    }

    return outputArray;
}