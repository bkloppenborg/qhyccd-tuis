#include <QCoreApplication>
#include <QDebug>
#include <qhyccd.h>
#include <string>
#include <thread>

#include <opencv2/highgui.hpp>
#include <opencv2/core/mat.hpp>

#include "cli_parser.hpp"

int main(int argc, char *argv[]) {
    using namespace std;

    // Configure the application
    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationName("Kloppenborg.net");
    QCoreApplication::setOrganizationDomain("kloppenbor.net");
    QCoreApplication::setApplicationName("qhyccd-tuis");

    unsigned int roiStartX = 0;
    unsigned int roiStartY = 0;
    unsigned int roiSizeX = 1024;
    unsigned int roiSizeY = 1024;
    unsigned int bpp;
    unsigned int channels;

    cv::Mat image_data(roiSizeX, roiSizeY, CV_16U);

    QMap<QString, QVariant> config = parse_cli(app);
    printConfig(config);

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

    bool filter_wheel_exists = IsQHYCCDCFWPlugged(handle);
    qDebug() << "Camera has filter wheel:" << filter_wheel_exists;
    int filter_wheel_max_slots = GetQHYCCDParam(handle, CONTROL_CFWSLOTSNUM);
    qDebug() << "Number of slots:" << filter_wheel_max_slots;

    // Set up the camera and take images.
    for(int idx = 0; idx < filters.length(); idx++) {

        int quantity = quantities[idx].toInt();
        double duration_usec = durations[idx].toDouble() * 1E6;
        double gain = gains[idx].toDouble();
        int offset = offsets[idx].toInt();
        QString filter_name = filters[idx];
        int filter_idx = filter_names.indexOf(filter_name);
        qDebug() << "Filter IDX:" << filter_idx;
        if(filter_idx == -1) {
            qWarning() << "Filter" << filter_name << "is not installed, skipping";
            continue;
        }
        
        // Change the filter
        if(filter_wheel_exists && filter_wheel_max_slots > 0) {
            char position[8] = {0};
            snprintf(position, 8, "%X", filter_idx);
            status = SendOrder2QHYCCDCFW(handle, position, 1);
            std::this_thread::sleep_for(1s);

            // Verify the filter changed
            status = GetQHYCCDCFWStatus(handle, position);
            qDebug() << filter_idx << position;
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

        // take images
        for(int exposure_idx = 0; exposure_idx < quantity; exposure_idx++) {
            qDebug() << "Starting exposure" << exposure_idx << "for" << duration_usec / 1E6 << "seconds";

            int64_t time_remaining_ms = duration_usec / 1E3;

            // Start the exposure
            const auto t_a = std::chrono::system_clock::now();
            status = ExpQHYCCDSingleFrame(handle);
            if(status != QHYCCD_SUCCESS) {
                qCritical() << "Exposure failed to start";
                exit(-1);
            }

            // Periodically wake up during the exposure to check for user commmands.
            do {
                std::this_thread::sleep_for(10ms);
                time_remaining_ms -= 100;
            } while (time_remaining_ms > 100);

            // Transfer the image. This is a blocking call.
            const auto t_b = std::chrono::system_clock::now();
            status = GetQHYCCDSingleFrame(handle, &roiSizeX, &roiSizeY, &bpp, &channels, image_data.ptr());
            const auto t_c = std::chrono::system_clock::now();
        }

    }

    // shutdown cleanly
    CloseQHYCCD(handle);
    ReleaseQHYCCDResource();

    return 0;
}