#include <QSettings>
#include <QtDebug>
#include <QVariant>
#include <QFileInfo>
#include <QDir>

#include "cli_parser.hpp"

// Function to dump the entire default contents of QSettings to a .ini file
void dumpDefaultConfigToFile(QMap<QString, QVariant> & config, const QString& filePath) {

    QSettings defaultSettings(filePath, QSettings::IniFormat);
    defaultSettings.setFallbacksEnabled(false);

    QStringList keys = config.keys();
    for (const QString &key : keys) {
        defaultSettings.setValue(key, config[key]);
    }
}

QStringList getTopLevelKeys(QMap<QString, QVariant> config) {
    QStringList allKeys = config.keys();

    QStringList topLevelKeys;

    for(const QString & key: allKeys) {
        if(!key.contains("/")) {
            topLevelKeys.append(key);
        }
    }

    return topLevelKeys;
}

void updateConfigFromCommandLine(QMap<QString, QVariant> & config, const QCommandLineParser& parser) {
    const QStringList optionNames = parser.optionNames();

    // Get the top-level keys in the QSettings variable
    QStringList topLevelKeys = getTopLevelKeys(config);

    for(const QString & key : topLevelKeys) {
        if(parser.isSet(key)) {
            config[key] = parser.value(key);
        }
    }
}

void updateConfigFromFile(QMap<QString, QVariant> & config, const QString& filePath) {
    QSettings fileSettings(filePath, QSettings::IniFormat);

    QStringList keys = fileSettings.allKeys();
    for (const QString& key : keys) {
        config[key] = fileSettings.value(key);
    }
}

void updateDefaultsFromSubConfig(QMap<QString, QVariant> & config, const QString & subConfig) {

    if(!subConfig.isEmpty()) {
    QStringList topLevelKeys = getTopLevelKeys(config);
    for(const QString & key: topLevelKeys) {
        const QString subKey = subConfig + "/" + key;
        if(config.contains(subKey)) {
            config[key] = config[subKey];
        }
    }
    }
}

void updateDefaults(QMap<QString, QVariant> & config, QCommandLineParser & parser, QString configName) {

    // First see if the configuration file has a corresponding block.
    // This block always exists as it is specified in parse_cli's config definition.
    QString subConfigName = config[configName].toString();
    if(!subConfigName.isEmpty()) {
        qDebug() << "Loading" << subConfigName;
        updateDefaultsFromSubConfig(config, subConfigName);
    }

    // Next check the command line interface
    QString parserBlock = parser.value(configName);
    if(!parserBlock.isEmpty()) {
        qDebug() << "Loading" << parserBlock;
        updateDefaultsFromSubConfig(config, parserBlock);
    }
}

void printConfig(const QMap<QString, QVariant> & config) {

    for(const QString & key: config.keys()) {
        qDebug() << key << " " << config[key];
    }
}

void checkIntegerType(const QString & str, const QString & errorMessage) {

    bool ok = true;

    str.toInt(&ok);
    if(!ok) {
        qCritical() << "Error:" << errorMessage;
        exit(-1);
    }
}

void checkIntegerType(const QStringList & list, const QString & errorMessage) {

    bool ok = true;

    for(const QString & str: list) {
        str.toInt(&ok);
        if(!ok) {
            qCritical() << "Error:" << errorMessage;
            exit(-1);
        }
    }
}

void checkNumericType(const QString & str, const QString & errorMessage) {

    bool ok = true;

    str.toDouble(&ok);
    if(!ok) {
        qCritical() << "Error:" << errorMessage;
        exit(-1);
    }
}

void checkNumericType(const QStringList & list, const QString & errorMessage) {

    bool ok = true;

    for(const QString & str: list) {
        str.toDouble(&ok);
        if(!ok) {
            qCritical() << "Error:" << errorMessage;
            exit(-1);
        }
    }
}

QStringList toStringList(const QVariant & var) {

    if(var.typeId() == QMetaType::QString) {
        return var.toString().split(",");
    }

    return var.toStringList();
}

void checkMatchingLength(const QStringList & A, const QStringList & B, const QString & errorMessage) {
    if(A.length() != B.length()) {
        qCritical() << errorMessage;
        exit(-1);
    }
}

QMap<QString, QVariant> parse_cli(const QCoreApplication & app) {

    // Configure the QMap with parameters that are relevant to the application
    QMap<QString, QVariant> config;
    config["config-file"] =  "";
    config["camera-config"] = "";
    config["exp-config"] = "";
    config["no-gui"] = "0";
    config["no-save"] = "0";
    config["save-dir"] = ".";

    // Site configurations, often specified in a site block.
    config["latitude"] = "0"; /// < Telescope latitude in degrees
    config["longitude"] = "0"; /// < Telescope longitude in degrees
    config["altitude"] = "0"; /// < Telescope altitude in degrees

    // Configuration options typically specified in a camera block
    config["camera-id"] =  "None";
    config["filter-names"] =  "None";   // an ordered list of filter names corresponding to slot numbers
    config["usb-transferbit"] =  "16";
    config["usb-traffic"] =  "0";
    config["camera-bin-mode"] = "1x1";
    config["camera-temperature"] = "40"; // Values >= 40 imply active cooling should be disabled.
    config["camera-cool-down"] = "0";
    config["camera-warm-up"] = "0";
    config["camera-cal-dir"] = "";

    // Configuration options typically specified in a exposure configuration block
    config["exp-quantities"] = "10";
    config["exp-durations"] = "1.0";
    config["exp-filters"] = "";
    config["exp-gains"] = "1.0";    // typically doesn't change between exposures, automatically replicated if needed.
    config["exp-offsets"] =  "30";  // typically doesn't change between exposures, automatically replicated if needed.

    // Configuration typically specified on the CLI
    config["catalog"] =  "None";
    config["object-id"] =  "None";

    // Set up a command line parser to accept a subset of the parameters.
    QCommandLineParser parser;
    parser.setApplicationDescription("Camera Configuration Example");
    parser.addHelpOption();

    // Broad configuration options.
    parser.addOption({{"config-file", "f"},     "Path to configuration file", "config-file"});
    parser.addOption({{"site-config", "sc"},  "Site configuration block name [optional]", "site-config"});
    parser.addOption({{"camera-config", "cc"},  "Camera configuration block name [optional]", "camera-config"});
    parser.addOption({{"exp-config", "ec"},     "Exposure configuration block name [optional]", "exp-config"});
    parser.addOption({"no-gui", "Disable all GUI elements"});   // boolean
    parser.addOption({{"no-save", "preview"}, "Disable saving FITS files"});   // boolean
    parser.addOption({{"save-dir", "sd"},       "Directory in which files will be saved", "save-dir"});

    // Site options
    parser.addOption({{"latitude", "lat"}, "Object identifier", "latitude"});
    parser.addOption({{"longitude", "lon"}, "Object identifier", "longitude"});
    parser.addOption({{"altitude", "alt"}, "Object identifier", "altitude"});

    // Camera options
    parser.addOption({"catalog", "Catalog name", "catalog"});
    parser.addOption({{"object-id", "object"}, "Object identifier", "object-id"});
    parser.addOption({"camera-id", "QHY Camera Identifier", "camera-id"});
    parser.addOption({"filter-names", "List of filters in the camera", ""});
    parser.addOption({"usb-traffic", "QHY USB Traffic Setting", "usb-traffic"});
    parser.addOption({"usb-transferbit", "Bits for image transfer. Options are 8 or 16", "usb-transferbit"});
    parser.addOption({{"camera-bin-mode", "cb"}, "Binning mode. Options: 1x1 - 9x9 further restricted by camera.", "camera-bin-mode"});
    parser.addOption({{"camera-temperature", "ct"}, "Set point for active cooling (Celsius)", "camera-temperature"});
    parser.addOption({{"camera-cool-down", "cool-down"}, "Instruct the camera to begin cooling to the temperature in `camera-temperature`."});
    parser.addOption({{"camera-warm-up", "warm-up", "cw"}, "Instruct the camera to begin warming up."});
    parser.addOption({{"camera-cal-dir", "cd"}, "Location for camera calibration images", "camera-cal-dir"});

    // Exposure options
    parser.addOption({{"exp-quantities", "eq"}, "Number of exposures per filter", "exp-quantities"});
    parser.addOption({{"exp-durations", "ed"},  "Exposure duration, in seconds, per filter", "exp-durations"});
    parser.addOption({{"exp-filters", "ef"},    "Names of filter to use", "exp-filters"});
    parser.addOption({{"exp-gains", "eg"},      "The gain to use per each filter", "exp-gains"});
    parser.addOption({{"exp-offsets", "eo"},    "Image offset per each filter", "exp-offsets"});

    // Display options
    parser.addOption({"draw-circle", "Draw a circle at the center of the image"}); // boolean

    // Other parameters
    parser.addOption(QCommandLineOption("dump-config", "Dump default configuration to file", "file"));

    // Parse the command line.
    parser.process(app);

    // Dump the configuration file if requested.
    if(parser.isSet("dump-config")) {
        QString filename = parser.value("dump-config");
        dumpDefaultConfigToFile(config, filename);
        exit(0);
    }

    // Update QSettings after reading the configuration file
    const QString configFile = parser.value("config-file");
    if (!configFile.isEmpty()) {
        qDebug() << "Loading settings from configuration file " << configFile;
        updateConfigFromFile(config, configFile);
        config["config-file"] = QVariant(configFile);
    }

    // Load pre-specified blocks for site, camera, and exposure configurations.
    updateDefaults(config, parser, "site-config");
    updateDefaults(config, parser, "camera-config");
    updateDefaults(config, parser, "exp-config");

    // Override config values with anything specified on the command line.
    qDebug() << "Updating settings from command line parameters.";
    updateConfigFromCommandLine(config, parser);

    //
    // Verify that the configuration makes sense
    //

    // Check application-wide settings
    if(parser.isSet("no-gui"))
        config["no-gui"] = "1";
    else
        config["no-gui"] = "0";

    if(parser.isSet("no-save"))
        config["no-save"] = "1";
    else
        config["no-save"] = "0";

    if(parser.isSet("draw-circle"))
        config["draw-circle"] = "1";
    else
        config["draw-circle"] = "0";


        // Check that the camera is specified
    if(config["camera-id"] == "None") {
        qCritical() << "Critical: Camera ID not specified. Exiting.";
        exit(-1);
    }

    // Convert the number of exposures to a QStringList. Verify that they are integers.
    const QStringList quantities = toStringList(config["exp-quantities"]);
    checkIntegerType(quantities, "exp-quantities must be a comma separated list of integer values without any spaces.");
    config["exp-quantities"] = quantities;

    // Convert the duration information to a QStringList. Enforce length matching to exposures.
    const QStringList durations = toStringList(config["exp-durations"]);
    checkNumericType(durations, "exp-durations must be a comma separated list of numeric values without any spaces");
    checkMatchingLength(quantities, durations, "The number of durations does match the number of exposures");
    config["exp-durations"] = durations;

    // Convert the filter information to a QStringList. Enforce length matching to exposures.
    QStringList exp_filters = toStringList(config["exp-filters"]);
    const QStringList filter_names = toStringList(config["filter-names"]);
    // If the user didn't specify a filter, select the default filter for them automatically
    if(exp_filters.length() == 1 && exp_filters[0] == "" && filter_names.length() > 0) {
        exp_filters[0] = filter_names[0];
    }
    checkMatchingLength(quantities, exp_filters, "The number of filters does match the number of exposures");
    config["exp-filters"] = exp_filters;

    // Convert the gain information to a QStringList. Require that at least one gain was specified.
    // Replicate the gain to all filters if necessary.
    QStringList gains = toStringList(config["exp-gains"]);
    checkNumericType(gains, "exp-gains must be a comma separated list of numeric values without any spaces");
    if(gains.length() == 0) {
        qCritical() << "The number of gains specified cannot be zero";
    }
    for(int i = gains.length(); i < quantities.length(); i++) {
        gains.append(gains.at(0));
    }
    config["exp-gains"] = gains;

    // Convert the offset information to a QStringList. Require that at least one gain was specified.
    // Replicate the offset to all filters if necessary.
    QStringList offsets = toStringList(config["exp-offsets"]);
    checkIntegerType(offsets, "exp-offsets must be a comma separated list of integer values without any spaces.");
    if(gains.length() == 0) {
        qCritical() << "The number of offsets specified cannot be zero";
        exit(-1);
    }
    for(int i = offsets.length(); i < quantities.length() ; i++) {
        offsets.append(offsets.at(0));
    }
    config["exp-offsets"] = offsets;

    // Check that the binning mode is allowed.
    QStringList allowed_bin_modes = {"1x1", "2x2", "3x3", "4x4", "5x5", "6x6", "7x7", "8x8", "9x9"};
    if(allowed_bin_modes.indexOf(config["camera-bin-mode"]) == -1) {
        qCritical() << "Binning mode must be one of " << allowed_bin_modes;
        exit(-1);
    }

    // Handle cooling / temperature settings.
    checkNumericType(config["camera-temperature"].toString(), "Camera temperature must be a numeric value.");
    // Ensure `camera-cool-down` and `camera-warm-up` are mutually exclusive.
    bool cool_down = parser.isSet("camera-cool-down");
    bool warm_up   = parser.isSet("camera-warm-up");
    if (warm_up) {
        config["camera-cool-down"] = "0";
        config["camera-warm-up"]   = "1";
        config["no-gui"] = "1"; // shut off the GUI, it isn't needed.
    } else if(cool_down) {
        config["camera-cool-down"] = "1";
        config["camera-warm-up"]   = "0";
        config["no-gui"] = "1"; // shut off the GUI, it isn't needed.
    }

    // Figure out where the camera stores its calibration files.
    QString cal_rel_dir = config["camera-cal-dir"].toString();
    if(cal_rel_dir.length() > 0) {
        QFileInfo configFile(config["config-file"].toString());
        QDir calDir(configFile.absoluteDir().absolutePath() + QDir::separator() + config["camera-cal-dir"].toString());
        config["camera-cal-dir"] = calDir.absolutePath();
    }

    // Resolve the save directory to an absolute path
    QFileInfo saveDir(config["save-dir"].toString() + QDir::separator());
    QDir calDir(saveDir.absoluteDir().absolutePath());
    config["save-dir"] = calDir.absolutePath() + QDir::separator();

    // Clean up the configuration by removing child configurations
    for(const QString & key: config.keys()) {
        if(key.contains("/")) {
            config.remove(key);
        }
    }

    return config;
}
