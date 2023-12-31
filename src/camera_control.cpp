#include <QApplication>
#include <QDebug>
#include <qhyccd.h>
#include <string>
#include <thread>
#include <signal.h>

#include <opencv2/highgui.hpp>
#include <opencv2/core/mat.hpp>
#include <opencv2/imgproc.hpp>

#include "cli_parser.hpp"

bool keep_running = true;

enum BayerOrder {
    BAYER_ORDER_GBRG,
    BAYER_ORDER_GRBG,
    BAYER_ORDER_BGGR,
    BAYER_ORDER_RGGB,
    BAYER_ORDER_NONE,
};

int takeExposures(const QMap<QString, QVariant> & config) {

    using namespace std;

    unsigned int roiStartX = 0;
    unsigned int roiStartY = 0;
    unsigned int roiSizeX = 1;
    unsigned int roiSizeY = 1;
    unsigned int bpp;
    unsigned int channels;
    BayerOrder bayer_order = BAYER_ORDER_NONE;


    // Unpack the configuration settings.
    string camera_id        = config["camera-id"].toString().toStdString();
    int usb_transferbit     = config["usb-transferbit"].toInt();
    int usb_traffic         = config["usb-traffic"].toInt();

    QStringList filter_names= config["filter-names"].toStringList(); 
    QStringList quantities  = config["exp-quantities"].toStringList();
    QStringList durations   = config["exp-durations"].toStringList();
    QStringList filters     = config["exp-filters"].toStringList();
    QStringList gains       = config["exp-gains"].toStringList();
    QStringList offsets     = config["exp-offsets"].toStringList();
    
    QString catalog_name    = config["catalog"].toString();
    QString object_name     = config["object-id"].toString();

    // Initalize the camera
    int status = QHYCCD_SUCCESS;
    status = InitQHYCCDResource();
    qhyccd_handle * handle = OpenQHYCCD((char*) camera_id.c_str());
    status = InitQHYCCD(handle);

    // Verify the camera supports the modes we will be using.
    status = IsQHYCCDControlAvailable(handle, CAM_SINGLEFRAMEMODE);
    if(status != QHYCCD_SUCCESS) {
        qCritical() << "Camera does not support single frame exposures";
        exit(-1);
    }

    // If this is a color camera, get the Bayer ordering.
    if(IsQHYCCDControlAvailable(handle, CAM_IS_COLOR) == QHYCCD_SUCCESS) {
        qDebug() << "Device is a color camera";
        int qhy_bayer_order = IsQHYCCDControlAvailable(handle, CAM_COLOR);
        switch(qhy_bayer_order) {
            case BAYER_GB:
                bayer_order = BAYER_ORDER_GBRG;
                qDebug() << "Bayer Order: BAYER_ORDER_GBRG";
            break;
            case BAYER_GR:
                bayer_order = BAYER_ORDER_GRBG;
                qDebug() << "Bayer Order: BAYER_ORDER_GRBG";
            break;
            case BAYER_BG:
                bayer_order = BAYER_ORDER_BGGR;
                qDebug() << "Bayer Order: BAYER_ORDER_BGGR";
            break;
            case BAYER_RG:
                bayer_order = BAYER_ORDER_RGGB;
                qDebug() << "Bayer Order: BAYER_ORDER_RGGB";
            break;
            default:
                bayer_order = BAYER_ORDER_NONE;
                qDebug() << "Bayer Order: BAYER_ORDER_NONE";
        }
    }

    // Get the maximum image size in 1x1 binning mode and set that as the default
    double chip_w, chip_h, pixel_w, pixel_h;
    uint32_t image_w, image_h, bit_depth;
    GetQHYCCDChipInfo(handle, &chip_w, &chip_h, &image_w, &image_h, &pixel_w, &pixel_h, &bit_depth);
    roiSizeX = image_w;
    roiSizeY = image_h;

    // Setup the filter wheel
    char fw_cmd_position[8] = {0};
    char fw_act_position[8] = {0};
    bool filter_wheel_exists = (IsQHYCCDCFWPlugged(handle) == QHYCCD_SUCCESS);
    int filter_wheel_max_slots = GetQHYCCDParam(handle, CONTROL_CFWSLOTSNUM);
    qDebug() << "Filter wheel exists?:" << filter_wheel_exists;
    qDebug() << "Filter wheel slots:" << filter_wheel_max_slots;

    // Set up the camera and take images.
    for(int idx = 0; keep_running && idx < filters.length(); idx++) {

        int quantity = quantities[idx].toInt();
        double duration_usec = durations[idx].toDouble() * 1E6;
        double gain = gains[idx].toDouble();
        int offset = offsets[idx].toInt();
        QString filter_name = filters[idx];
        int filter_idx = filter_names.indexOf(filter_name);
        if(filter_idx == -1) {
            qWarning() << "Filter" << filter_name << "is not installed, skipping";
            continue;
        }
        
        // Change the filter
        if(filter_wheel_exists && filter_wheel_max_slots > 0) {
            qDebug() << "Commanding filter wheel to change to" << filter_name << "slot" << filter_idx;

            snprintf(fw_cmd_position, 8, "%X", filter_idx);
            status = SendOrder2QHYCCDCFW(handle, fw_cmd_position, 1);

            do {
                std::this_thread::sleep_for(500ms);
                status = GetQHYCCDCFWStatus(handle, fw_act_position);

            } while (strcmp(fw_cmd_position, fw_act_position) != 0);

            qDebug() << "Filter change to" << filter_name << "successful";
        }

        // Configure the camera
        status  = SetQHYCCDStreamMode(handle, 0);
        status |= SetQHYCCDParam(handle, CONTROL_TRANSFERBIT, usb_transferbit);
        status |= SetQHYCCDParam(handle, CONTROL_USBTRAFFIC, usb_traffic);
        status |= SetQHYCCDParam(handle, CONTROL_GAIN, gain);
        status |= SetQHYCCDParam(handle, CONTROL_OFFSET, offset);
        status |= SetQHYCCDParam(handle, CONTROL_EXPOSURE, duration_usec);
        status |= SetQHYCCDResolution(handle, roiStartX, roiStartY, roiSizeX, roiSizeY);
        status |= SetQHYCCDBinMode(handle, 1, 1);
        status |= SetQHYCCDBitsMode(handle, 16);
        if(status != QHYCCD_SUCCESS) {
            qCritical() << "Camera configuration failed";
            exit(-1);
        }

        // Allocate a buffers to store the images (temporary)
        cv::Mat raw_image(roiSizeY, roiSizeX, CV_16U);
        cv::Mat color_image(raw_image.rows / 2, raw_image.cols / 2, CV_16UC3);
        cv::Mat display_image;

        // take images
        for(int exposure_idx = 0; keep_running && exposure_idx < quantity; exposure_idx++) {
            qDebug() << "Starting exposure" << exposure_idx + 1 << "/" << quantity
                     << "with a duration of" << duration_usec / 1E6 << "seconds";

            int64_t time_remaining_ms = duration_usec / 1E3;

            // Start the exposure
            const auto t_a = std::chrono::system_clock::now();
            status = ExpQHYCCDSingleFrame(handle);
            if(status != QHYCCD_SUCCESS) {
                qCritical() << "Exposure failed to start";
                exit(-1);
            }

            // Wake up every 10 milliseconds to check on exposure progress.
            do {
                std::this_thread::sleep_for(10ms);
                time_remaining_ms -= 10;
            } while (keep_running && time_remaining_ms > 100);

            // If we are instructed to exit, abort the exposure and readout.
            if(!keep_running) {
                qDebug() << "Aborting exposure and readout";
                status = CancelQHYCCDExposingAndReadout(handle);
                break;
            }

            // Transfer the image. This is a blocking call.
            const auto t_b = std::chrono::system_clock::now();
            status = GetQHYCCDSingleFrame(handle, &roiSizeX, &roiSizeY, &bpp, &channels, raw_image.ptr());
            const auto t_c = std::chrono::system_clock::now();

            // De-bayer the image if needed
            if(bayer_order == BAYER_ORDER_GBRG) {
                cv::cvtColor(raw_image, color_image, cv::COLOR_BayerGBRG2BGR);
                display_image = color_image;
            } else if (bayer_order == BAYER_ORDER_GRBG) {
                cv::cvtColor(raw_image, color_image, cv::COLOR_BayerGRBG2BGR);
                display_image = color_image;
            } else if (bayer_order == BAYER_ORDER_BGGR) {
                cv::cvtColor(raw_image, color_image, cv::COLOR_BayerBGGR2BGR);
                display_image = color_image;
            } else if (bayer_order == BAYER_ORDER_RGGB) {
                cv::cvtColor(raw_image, color_image, cv::COLOR_BayerRGGB2BGR);
                display_image = color_image;
            } else {
                // not a bayer image, just swap buffers
                display_image = raw_image;
            }

            // Show the image.
            cv::imshow("display_window", display_image);
            cv::waitKey(1);
        }
    }

    // shutdown cleanly
    CloseQHYCCD(handle);
    ReleaseQHYCCDResource();

    return 0;
}
