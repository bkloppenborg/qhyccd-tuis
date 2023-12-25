#include <iostream>
#include <iomanip>

#include "version.h"
#include "qhyccd.h"

void print_control_header() {
    using namespace std;
    cout << "\n" << "  "
        << setw(36) << std::left << "Control Name"
        << setw(11) << std::left << "Supported?"
        << setw(8) << std::right << "Minimum"
        << setw(8) << std::right << "Maximum"
        << setw(8) << std::right << "Step"
        << endl;
}

void check_control(qhyccd_handle * handle, CONTROL_ID control_id, const char* control_name) {
    using namespace std;

    if(IsQHYCCDControlAvailable(handle, control_id) == QHYCCD_SUCCESS) {
        double minimum = 0, maximum = 0, step = 0;
        GetQHYCCDParamMinMaxStep(handle, control_id, &minimum, &maximum, &step);

        cout << "  "
            << setw(36) << std::left << control_name
            << setw(11) << std::left << "Yes"
            << setw(8) << std::right << minimum
            << setw(8) << std::right << maximum
            << setw(8) << std::right << step
            << endl;
    } else {
        cout << "  "
            << setw(36) << std::left << control_name
            << setw(11) << std::left << "No"
            << setw(8) << std::right << "-"
            << setw(8) << std::right << "-"
            << setw(8) << std::right << "-"
            << endl;
    }
}

int main(int argc, char *argv[])
{
    using namespace std;

    int num_cameras = 0;
    char camera_id[CAMERA_ID_LENGTH];
    char camera_model[64];
    double min, max, step;
    CONTROL_ID control_id;
    uint32_t num_modes;
    char read_mode_name[MAX_READMODE_NAME];
    char cfw_status;
    uint32_t cfw_max_filters;
    double chip_w, chip_h, pixel_w, pixel_h;
    uint32_t image_w, image_h, bit_depth;
    uint8_t fw_version[16];
    char sensor_name[256];

    // Initialize the QHY Library.
    InitQHYCCDResource();

    num_cameras = ScanQHYCCD();

    for(uint32_t camera_idx = 0; camera_idx < num_cameras; camera_idx++) {
        cout << "----------------------------------------------------" << endl;

        GetQHYCCDId(camera_idx, camera_id);
        cout << "Camera ID   : " << camera_id << endl;

        GetQHYCCDModel(camera_id, camera_model);
        cout << " Camera Model: " << camera_model << endl;

        qhyccd_handle * handle = OpenQHYCCD(camera_id);
        InitQHYCCD(handle);

        GetQHYCCDFWVersion(handle, fw_version);
        cout << " Firmware Version: " << fw_version << endl;

        GetQHYCCDSensorName(handle, sensor_name);
        cout << " Sensor Name: " << sensor_name << endl;

        GetQHYCCDChipInfo(handle, &chip_w, &chip_h, &image_w, &image_h, &pixel_w, &pixel_h, &bit_depth);
        cout << " Chip Size: " << chip_w << " x " << chip_h << " mm" << endl;
        cout << " Image Size: " << image_w << " x " << image_h << endl;
        cout << " Pixel Size: " << pixel_w << " x " << pixel_h << " um" << endl;
        cout << " Bit Depth: " << bit_depth << endl;

        // Different read modes
        GetQHYCCDNumberOfReadModes(handle, &num_modes);
        cout << " Read modes: " << num_modes << endl;
        for(uint32_t mode_idx = 0; mode_idx < num_modes; mode_idx++) {
            GetQHYCCDReadModeName(handle, mode_idx, read_mode_name);
            cout << "  " << mode_idx << ": " << read_mode_name << endl;
        }

        // Size of the image in MB
        int mem_size = GetQHYCCDMemLength(handle);
        cout << " Image size: " << mem_size / 1024 / 1024 << " MB" << endl;

        uint32_t has_filter_wheel = IsQHYCCDCFWPlugged(handle);
        if(has_filter_wheel == QHYCCD_SUCCESS) {
            cfw_max_filters = GetQHYCCDParam(handle, CONTROL_CFWSLOTSNUM);
            GetQHYCCDCFWStatus(handle, &cfw_status);
            cout << " Filter wheel: detected" << endl;
            cout << "  Slots: " << cfw_max_filters << endl;
            cout << "  Current Slot: " << cfw_status << endl;
        } else {
            cout << " Filter wheel: not detected" << endl;
        }

        cout << " Possible Controls:" << endl;
        print_control_header();
        check_control(handle, CONTROL_BRIGHTNESS, "CONTROL_BRIGHTNESS");
        check_control(handle, CONTROL_CONTRAST, "CONTROL_CONTRAST");
        check_control(handle, CONTROL_WBR, "CONTROL_WBR");
        check_control(handle, CONTROL_WBB, "CONTROL_WBB");
        check_control(handle, CONTROL_WBG, "CONTROL_WBG");
        check_control(handle, CONTROL_GAMMA, "CONTROL_GAMMA");
        check_control(handle, CONTROL_GAIN, "CONTROL_GAIN");
        check_control(handle, CONTROL_OFFSET, "CONTROL_OFFSET");
        check_control(handle, CONTROL_EXPOSURE, "CONTROL_EXPOSURE");
        check_control(handle, CONTROL_SPEED, "CONTROL_SPEED");
        check_control(handle, CONTROL_TRANSFERBIT, "CONTROL_TRANSFERBIT");
        check_control(handle, CONTROL_CHANNELS, "CONTROL_CHANNELS");
        check_control(handle, CONTROL_USBTRAFFIC, "CONTROL_USBTRAFFIC");
        check_control(handle, CONTROL_ROWNOISERE, "CONTROL_ROWNOISERE");
        check_control(handle, CONTROL_CURTEMP, "CONTROL_CURTEMP");
        check_control(handle, CONTROL_CURPWM, "CONTROL_CURPWM");
        check_control(handle, CONTROL_MANULPWM, "CONTROL_MANULPWM");
        check_control(handle, CONTROL_CFWPORT, "CONTROL_CFWPORT");
        check_control(handle, CONTROL_COOLER, "CONTROL_COOLER");
        check_control(handle, CONTROL_ST4PORT, "CONTROL_ST4PORT");
        check_control(handle, CAM_COLOR, "CAM_COLOR");
        check_control(handle, CAM_BIN1X1MODE, "CAM_BIN1X1MODE");
        check_control(handle, CAM_BIN2X2MODE, "CAM_BIN2X2MODE");
        check_control(handle, CAM_BIN3X3MODE, "CAM_BIN3X3MODE");
        print_control_header();
        check_control(handle, CAM_BIN4X4MODE, "CAM_BIN4X4MODE");
        check_control(handle, CAM_MECHANICALSHUTTER, "CAM_MECHANICALSHUTTER");
        check_control(handle, CAM_TRIGER_INTERFACE, "CAM_TRIGER_INTERFACE");
        check_control(handle, CAM_TECOVERPROTECT_INTERFACE, "CAM_TECOVERPROTECT_INTERFACE");
        check_control(handle, CAM_SINGNALCLAMP_INTERFACE, "CAM_SINGNALCLAMP_INTERFACE");
        check_control(handle, CAM_FINETONE_INTERFACE, "CAM_FINETONE_INTERFACE");
        check_control(handle, CAM_SHUTTERMOTORHEATING_INTERFACE, "CAM_SHUTTERMOTORHEATING_INTERFACE");
        check_control(handle, CAM_CALIBRATEFPN_INTERFACE, "CAM_CALIBRATEFPN_INTERFACE");
        check_control(handle, CAM_CHIPTEMPERATURESENSOR_INTERFACE, "CAM_CHIPTEMPERATURESENSOR_INTERFACE");
        check_control(handle, CAM_USBREADOUTSLOWEST_INTERFACE, "CAM_USBREADOUTSLOWEST_INTERFACE");

        check_control(handle, CAM_8BITS, "CAM_8BITS");
        check_control(handle, CAM_16BITS, "CAM_16BITS");
        check_control(handle, CAM_GPS, "CAM_GPS");

        check_control(handle, CAM_IGNOREOVERSCAN_INTERFACE, "CAM_IGNOREOVERSCAN_INTERFACE");

        check_control(handle, CAM_CurveSystemGain, "CAM_CURVESYSTEMGAIN");
        check_control(handle, CAM_CurveFullWell, "CAM_CURVEFULLWELL");
        check_control(handle, CAM_CurveReadoutNoise, "CAM_CURVEREADOUTNOISE");

        check_control(handle, QHYCCD_3A_AUTOEXPOSURE, "QHYCCD_3A_AUTOEXPOSURE");
        check_control(handle, QHYCCD_3A_AUTOFOCUS, "QHYCCD_3A_AUTOFOCUS");
        check_control(handle, CONTROL_AMPV, "CONTROL_AMPV");
        check_control(handle, CONTROL_VCAM, "CONTROL_VCAM");
        check_control(handle, CAM_VIEW_MODE, "CAM_VIEW_MODE");

        print_control_header();
        check_control(handle, CONTROL_CFWSLOTSNUM, "CONTROL_CFWSLOTSNUM");
        check_control(handle, IS_EXPOSING_DONE, "IS_EXPOSING_DONE");
        check_control(handle, ScreenStretchB, "SCREENSTRETCHB");
        check_control(handle, ScreenStretchW, "SCREENSTRETCHW");
        check_control(handle, CONTROL_DDR, "CONTROL_DDR");
        check_control(handle, CAM_LIGHT_PERFORMANCE_MODE, "CAM_LIGHT_PERFORMANCE_MODE");

        check_control(handle, CAM_QHY5II_GUIDE_MODE, "CAM_QHY5II_GUIDE_MODE");
        check_control(handle, DDR_BUFFER_CAPACITY, "DDR_BUFFER_CAPACITY");
        check_control(handle, DDR_BUFFER_READ_THRESHOLD, "DDR_BUFFER_READ_THRESHOLD");
        check_control(handle, DefaultGain, "DEFAULTGAIN");
        check_control(handle, DefaultOffset, "DEFAULTOFFSET");
        check_control(handle, OutputDataActualBits, "OUTPUTDATAACTUALBITS");
        check_control(handle, OutputDataAlignment, "OUTPUTDATAALIGNMENT");

        check_control(handle, CAM_SINGLEFRAMEMODE, "CAM_SINGLEFRAMEMODE");
        check_control(handle, CAM_LIVEVIDEOMODE, "CAM_LIVEVIDEOMODE");
        check_control(handle, CAM_IS_COLOR, "CAM_IS_COLOR");
        check_control(handle, hasHardwareFrameCounter, "HASHARDWAREFRAMECOUNTER");
        check_control(handle, CONTROL_MAX_ID_Error, "CONTROL_MAX_ID_ERROR");
        check_control(handle, CAM_HUMIDITY, "CAM_HUMIDITY");
        check_control(handle, CAM_PRESSURE, "CAM_PRESSURE");
        check_control(handle, CONTROL_VACUUM_PUMP, "CONTROL_VACUUM_PUMP");
        check_control(handle, CONTROL_SensorChamberCycle_PUMP, "CONTROL_SENSORCHAMBERCYCLE_PUMP");

        print_control_header();
        check_control(handle, CAM_32BITS, "CAM_32BITS");
        check_control(handle, CAM_Sensor_ULVO_Status, "CAM_SENSOR_ULVO_STATUS");
        check_control(handle, CAM_SensorPhaseReTrain, "CAM_SENSORPHASERETRAIN");
        check_control(handle, CAM_InitConfigFromFlash, "CAM_INITCONFIGFROMFLASH");
        check_control(handle, CAM_TRIGER_MODE, "CAM_TRIGER_MODE");
        check_control(handle, CAM_TRIGER_OUT, "CAM_TRIGER_OUT");
        check_control(handle, CAM_BURST_MODE, "CAM_BURST_MODE");
        check_control(handle, CAM_SPEAKER_LED_ALARM, "CAM_SPEAKER_LED_ALARM");
        check_control(handle, CAM_WATCH_DOG_FPGA, "CAM_WATCH_DOG_FPGA");

        check_control(handle, CAM_BIN6X6MODE, "CAM_BIN6X6MODE");
        check_control(handle, CAM_BIN8X8MODE, "CAM_BIN8X8MODE");
        check_control(handle, CAM_GlobalSensorGPSLED, "CAM_GLOBALSENSORGPSLED");
        check_control(handle, CONTROL_ImgProc, "CONTROL_IMGPROC");
        check_control(handle, CONTROL_RemoveRBI, "CONTROL_REMOVERBI");
        check_control(handle, CONTROL_GlobalReset, "CONTROL_GLOBALRESET");
        check_control(handle, CONTROL_FrameDetect, "CONTROL_FRAMEDETECT");
        check_control(handle, CAM_GainDBConversion, "CAM_GAINDBCONVERSION");
        check_control(handle, CAM_CurveSystemGain, "CAM_CURVESYSTEMGAIN");
        check_control(handle, CAM_CurveFullWell, "CAM_CURVEFULLWELL");
        check_control(handle, CAM_CurveReadoutNoise, "CAM_CURVEREADOUTNOISE");

        check_control(handle, CONTROL_MAX_ID, "CONTROL_MAX_ID");

        check_control(handle, CONTROL_AUTOWHITEBALANCE, "CONTROL_AUTOWHITEBALANCE");
        check_control(handle, CONTROL_AUTOEXPOSURE, "CONTROL_AUTOEXPOSURE");

        print_control_header();
        check_control(handle, CONTROL_AUTOEXPmessureValue, "CONTROL_AUTOEXPMESSUREVALUE");
        check_control(handle, CONTROL_AUTOEXPmessureMethod, "CONTROL_AUTOEXPMESSUREMETHOD");
        check_control(handle, CONTROL_ImageStabilization, "CONTROL_IMAGESTABILIZATION");
        check_control(handle, CONTROL_GAINdB, "CONTROL_GAINDB");
        check_control(handle, CONTROL_DPC, "CONTROL_DPC");
        check_control(handle, CONTROL_DPC_value, "CONTROL_DPC_VALUE");

        // Close this camera.
        CloseQHYCCD(handle);
    }

    // Release QHYCCD SDK resources
    ReleaseQHYCCDResource();
}


