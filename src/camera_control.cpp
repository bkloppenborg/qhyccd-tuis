#include <QApplication>
#include <QDebug>
#include <QDateTime>
#include <QFileInfo>
#include <QDir>

#include <string>
#include <thread>
#include <signal.h>

#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/core/mat.hpp>
#include <opencv2/imgproc.hpp>

#include "camera_control.hpp"
#include "cli_parser.hpp"
#include "cvfits.hpp"
#include "image_calibration.hpp"

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

    double latitude = 0;
    double longitude = 0;
    double altitude = 0;

    double temperature = -999;

    uint32_t roiStartX = 0;
    uint32_t roiStartY = 0;
    uint32_t roiSizeX = 1;
    uint32_t roiSizeY = 1;
    uint32_t retSizeX = 1;
    uint32_t retSizeY = 1;
    uint32_t bpp;
    uint32_t channels;
    BayerOrder bayer_order = BAYER_ORDER_NONE;

    // Unpack application settings
    bool enable_gui = (config["no-gui"] == "0");
    bool save_fits =  (config["no-save"] == "0");
    QString save_dir        = config["save-dir"].toString();

    // Unpack the camera configuration settings
    string camera_id        = config["camera-id"].toString().toStdString();
    int usb_transferbit     = config["usb-transferbit"].toInt();
    int usb_traffic         = config["usb-traffic"].toInt();
    QStringList filter_names= config["filter-names"].toStringList();
    QString cal_dir         = config["camera-cal-dir"].toString();
    QString requestedBinMode = config["camera-bin-mode"].toString();
    QString setBinMode      = "1x1";
    int binX = 1;
    int binY = 1;

    // Unpack exposure configuration settings.
    QStringList quantities  = config["exp-quantities"].toStringList();
    QStringList durations   = config["exp-durations"].toStringList();
    QStringList filters     = config["exp-filters"].toStringList();
    QStringList gains       = config["exp-gains"].toStringList();
    QStringList offsets     = config["exp-offsets"].toStringList();

    // Unpack object information. Replace spaces with underscores.
    QString catalog_name    = config["catalog"].toString();
    QString object_id       = config["object-id"].toString();
    std::replace(object_id.begin(), object_id.end(), ' ', '_');

    // Unpack optional settings
    bool draw_circle = config["draw-circle"].toBool();

    // Initalize the camera
    int status = QHYCCD_SUCCESS;
    status = InitQHYCCDResource();
    qhyccd_handle * handle = OpenQHYCCD((char*) camera_id.c_str());

    // Set to single frame mode.
    status = SetQHYCCDStreamMode(handle, 0);

    status = InitQHYCCD(handle);
    if(status != QHYCCD_SUCCESS) {
        qCritical() << "Camera cannot be initialized. Is it plugged in?";
        exit(-1);
    }

    // Verify the camera supports the modes we will be using.
    status = IsQHYCCDControlAvailable(handle, CAM_SINGLEFRAMEMODE);
    if(status != QHYCCD_SUCCESS) {
        qCritical() << "Camera does not support single frame exposures";
        exit(-1);
    }

    // Determine if we can get the temperature
    bool can_get_temperature = (IsQHYCCDControlAvailable(handle, CONTROL_CURTEMP) == QHYCCD_SUCCESS);

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

    // Get the maximum image size, ignoring the overscan area, in 1x1 binning mode.
    // Use this as the default image size.
    GetQHYCCDEffectiveArea(handle, &roiStartX, &roiStartY, &roiSizeX, &roiSizeY);

    // Setup the filter wheel
    char fw_cmd_position[8] = {0};
    char fw_act_position[8] = {0};
    bool filter_wheel_exists = (IsQHYCCDCFWPlugged(handle) == QHYCCD_SUCCESS);
    qDebug() << "Filter wheel exists?:" << filter_wheel_exists;
    int filter_wheel_max_slots = 0;
    if(filter_wheel_exists) {
        filter_wheel_max_slots = GetQHYCCDParam(handle, CONTROL_CFWSLOTSNUM);
        qDebug() << "Filter wheel slots:" << filter_wheel_max_slots;
    }

    // Configure camera settings that are in common to all images
    status  = SetQHYCCDParam(handle, CONTROL_TRANSFERBIT, usb_transferbit);
    status |= SetQHYCCDParam(handle, CONTROL_USBTRAFFIC, usb_traffic);
    status |= SetQHYCCDResolution(handle, roiStartX, roiStartY, roiSizeX, roiSizeY);
    status |= setCameraBinMode(handle, requestedBinMode, setBinMode, binX, binY);
    status |= SetQHYCCDBitsMode(handle, 16);
    if(status != QHYCCD_SUCCESS) {
        qCritical() << "Camera configuration failed";
        exit(-1);
    }

    // Calculate the size of the resulting image
    uint32_t imageSizeX = roiSizeX / binX;
    uint32_t imageSizeY = roiSizeY / binY;

    // Allocate a buffers to store the images
    cv::Mat raw_image(imageSizeY, imageSizeX, CV_16U);
    cv::Mat color_image(imageSizeY / 2, imageSizeX / 2, CV_16UC3);
    cv::Mat display_image;

    CVFITS cvfits;

    cv::Point2d image_center(imageSizeX / 2, imageSizeY / 2);
    cv::Scalar white_color(255, 255, 255);
    cv::Scalar black_color(0,0,0);

    // Set up the camera and take images.
    for(int idx = 0; keep_running && idx < filters.length(); idx++) {

        int quantity = quantities[idx].toInt();
        double duration_sec  = durations[idx].toDouble();
        double duration_usec = duration_sec * 1E6;
        double gain = gains[idx].toDouble();
        int offset = offsets[idx].toInt();
        QString filter_name = filters[idx];
        int filter_idx = filter_names.indexOf(filter_name);
        if(filter_idx == -1) {
            qWarning() << "Filter" << filter_name << "is not installed, skipping";
            continue;
        }

        // Load the flat file for this filter
        cv::Mat flat_image = cv::Mat::ones(imageSizeY, imageSizeX, CV_16U);
        //QString flatFileName = cal_dir + QDir::separator() + "average_flat_" + filter_name + ".fits";
        QString flatFileName = "None.fits";
        QFileInfo flatFileInfo(flatFileName);
        if(flatFileInfo.exists() && flatFileInfo.isFile()) {
            // Load the image and scale it to the image duration
            qDebug() << "Loading" << flatFileName;
            CVFITS cvFlat(flatFileName.toStdString());
            cv::multiply(cvFlat.image, duration_sec, flat_image);
        }

        // Configure exposure settings unique to this filter.
        status |= SetQHYCCDParam(handle, CONTROL_GAIN, gain);
        status |= SetQHYCCDParam(handle, CONTROL_OFFSET, offset);
        status |= SetQHYCCDParam(handle, CONTROL_EXPOSURE, duration_usec);

        // Change the filter
        if(filter_wheel_exists && filter_wheel_max_slots > 0) {
            qDebug() << "Commanding filter wheel to change to" << filter_name << "slot" << filter_idx;

            snprintf(fw_cmd_position, 8, "%X", filter_idx);
            status = SendOrder2QHYCCDCFW(handle, fw_cmd_position, 1);

            do {
                std::this_thread::sleep_for(500ms);
                status = GetQHYCCDCFWStatus(handle, fw_act_position);

            } while (strcmp(fw_cmd_position, fw_act_position) != 0);

            // Wait an additional second for the filter wheel motion to complete.
            std::this_thread::sleep_for(1s);

            qDebug() << "Filter change to" << filter_name << "successful";
        }

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
            status = GetQHYCCDSingleFrame(handle, &retSizeX, &retSizeY, &bpp, &channels, raw_image.ptr());
            const auto t_c = std::chrono::system_clock::now();

            if(roiSizeX / binX != retSizeX || roiSizeY / binY != retSizeY) {
                qFatal("Predicted vs. actual image size mismatch!");
            }

            // Get additional time-dependent information from the camera.
            if(can_get_temperature)
                temperature = GetQHYCCDParam(handle, CONTROL_CURTEMP);

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

            // Save FITS files when instructed.
            if(save_fits) {
                // Save the image data
                QString filename = QDateTime::currentDateTimeUtc().toString(Qt::ISODate) +
                    "_" + catalog_name + "_" + object_id + "_" + filter_name + ".fits";
                // replace colons in the filename with hypens
                std::replace(filename.begin(), filename.end(), ':', '-');

                QString full_path = save_dir + filename;

                cvfits.image = display_image;
                cvfits.detector_name = camera_id;
                cvfits.filter_name = filter_name.toStdString();
                cvfits.bin_mode_name = setBinMode.toStdString();
                cvfits.xbinning = binX;
                cvfits.ybinning = binY;
                cvfits.exposure_start = t_a;
                cvfits.exposure_end = t_b;
                cvfits.readout_start = t_b;
                cvfits.readout_end = t_c;
                cvfits.exposure_duration_sec = duration_sec;
                cvfits.catalog_name = catalog_name.toStdString();
                cvfits.object_name = object_id.toStdString();
                cvfits.latitude = latitude;
                cvfits.longitude = longitude;
                cvfits.altitude = altitude;
                cvfits.temperature = temperature;
                cvfits.gain = gain;

                cvfits.saveToFITS(full_path.toStdString());
            }

            // Display the image when instructed.
            if(enable_gui) {

                //display_image /= flat_image;
                display_image = scaleImageLinear(display_image);

                // Draw a circle for the image center.
                if(draw_circle) {
                    int inner_ring = 50 / binX;
                    int ring_width = 10 / binX;
                    int outer_ring = 100 / binX;

                    cv::circle(display_image, image_center, inner_ring, white_color, ring_width);
                    cv::circle(display_image, image_center, inner_ring + ring_width, black_color, ring_width);
                    cv::circle(display_image, image_center, outer_ring, white_color, ring_width);
                    cv::circle(display_image, image_center, outer_ring + ring_width, black_color, ring_width);
                }

                // Show the image.
                cv::imshow("display_window", display_image);
                cv::waitKey(1);
            }
        }
    }

    // shutdown cleanly
    CloseQHYCCD(handle);
    ReleaseQHYCCDResource();

    return 0;
}

int setCameraBinMode(qhyccd_handle * handle, const QString & requestedMode, QString & setMode, int & binX, int & binY) {

    // default to 1x1 binning
    binX = 1;
    binY = 1;
    CONTROL_ID control_id = CAM_BIN1X1MODE;
    setMode = "1x1";

    // Pick the options for binning modes:
    if (requestedMode == "2x2") {
        binX = 2;
        binY = 2;
        control_id = CAM_BIN2X2MODE;
        setMode = requestedMode;
    } else if (requestedMode == "3x3") {
        binX = 3;
        binY = 3;
        control_id = CAM_BIN3X3MODE;
        setMode = requestedMode;
    } else if (requestedMode == "4x4") {
        binX = 4;
        binY = 4;
        control_id = CAM_BIN4X4MODE;
        setMode = requestedMode;
    } else if (requestedMode == "5x5") {
        qWarning() << "Warning: 5x5 binning is NOT supported. Defaulting to 4x4 binning";
        binX = 4;
        binY = 4;
        control_id = CAM_BIN4X4MODE;
        setMode = "4x4";
    } else if (requestedMode == "6x6") {
        binX = 6;
        binY = 6;
        control_id = CAM_BIN6X6MODE;
        setMode = requestedMode;
    } else if (requestedMode == "7x7") {
        qWarning() << "Warning: 7x7 binning is NOT supported. Defaulting to 6x6 binning";
        binX = 6;
        binY = 6;
        control_id = CAM_BIN6X6MODE;
        setMode = "6x6";
    } else if (requestedMode == "8x8") {
        binX = 8;
        binY = 8;
        control_id = CAM_BIN8X8MODE;
        setMode = requestedMode;
    } else if (requestedMode == "9x9") {
        qWarning() << "Warning: 9x9 binning is NOT supported. Defaulting to 8x8 binning";
        binX = 8;
        binY = 8;
        control_id = CAM_BIN8X8MODE;
        setMode = "8x8";
    }

    // We treat 1x1 binning as a special fallback mode. Handle that case separately.
    if(control_id == CAM_BIN1X1MODE) {
        return SetQHYCCDBinMode(handle, binX, binY);
    }

    // For all other binning modes, verify that the binning mode is supported.
    // If it isn't supported, issue a warning and call this function recursively to set things.
    bool modeSupported = (IsQHYCCDControlAvailable(handle, control_id) == QHYCCD_SUCCESS);
    if(!modeSupported) {
        qWarning() << "Warning: Binning" << requestedMode << "is not supported, reverting to 1x1 binning";
        return setCameraBinMode(handle, "1x1", setMode, binX, binY);
    }

    // If the binning mode is supported, go ahead and set it.
    qDebug() << "Setting bin mode to " << setMode;
    return SetQHYCCDBinMode(handle, binX, binY);
}

void setTemperature(qhyccd_handle * handle, double setPointC) {

    int status = QHYCCD_SUCCESS;
    status  = IsQHYCCDControlAvailable(handle, CONTROL_COOLER);

    if(status == QHYCCD_SUCCESS) {
        SetQHYCCDParam(handle, CONTROL_COOLER, setPointC);
    } else {
        qWarning() << "Camera does not support cooling";
    }
}

void monitorTemperature(qhyccd_handle * handle) {
    using namespace std;

    int status = QHYCCD_SUCCESS;
    status  = IsQHYCCDControlAvailable(handle, CONTROL_COOLER);
    status |= IsQHYCCDControlAvailable(handle, CONTROL_CURTEMP);

    double temperature = -999;

    if(status == QHYCCD_SUCCESS) {
        while(keep_running) {
            temperature = GetQHYCCDParam(handle, CONTROL_CURTEMP);
            qDebug() << "Temperature:" << temperature;
            std::this_thread::sleep_for(2s);
        }
    }
}

int runCooler(const QMap<QString, QVariant> & config) {
    using namespace std;

    bool cool_down   = (config["camera-cool-down"].toString() == "1");
    double temperature = config["camera-temperature"].toDouble();
    string camera_id   = config["camera-id"].toString().toStdString();

    // Initalize the camera
    int status = QHYCCD_SUCCESS;
    status = InitQHYCCDResource();
    qhyccd_handle * handle = OpenQHYCCD((char*) camera_id.c_str());

    if(cool_down) {
        qDebug() << "Starting camera cooler";
    } else {
        qDebug() << "Disabling camera cooler";
        temperature = 40.0;
    }

    status  = InitQHYCCD(handle);
    status |= IsQHYCCDControlAvailable(handle, CONTROL_COOLER);
    status |= IsQHYCCDControlAvailable(handle, CONTROL_CURTEMP);

    if(status != QHYCCD_SUCCESS) {
        qCritical() << "Camera does not support cooling. Aborting.";
        return -1;
    }

    qDebug() << "Setting temperature to" << temperature;
    setTemperature(handle, temperature);
    //monitorTemperature(handle);

    // shutdown cleanly
    CloseQHYCCD(handle);
    ReleaseQHYCCDResource();

    return 0;
}