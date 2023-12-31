#include <QSettings>
#include <QtDebug>
#include <QVariant>

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

    // Configuration options typically specified in a camera block
    config["camera-id"] =  "None";
    config["filter-names"] =  "None";   // an ordered list of filter names corresponding to slot numbers
    config["usb-transferbit"] =  "16";
    config["usb-traffic"] =  "10";
    config["camera-bin-mode"] = "1x1";
    config["camera-temperature"] = "40"; // Values >= 40 imply active cooling should be disabled.
    config["camera-cool-down"] = "0";
    config["camera-warm-up"] = "0";

    // Configuration options typically specified in a exposure configuration block
    config["exp-quantities"] = "10";
    config["exp-durations"] = "1.0";
    config["exp-filters"] = "";
    config["exp-gains"] = "1.0";    // typically doesn't change between exposures, automatically replicated if needed.
    config["exp-offsets"] =  "100";  // typically doesn't change between exposures, automatically replicated if needed.

    // Configuration typically specified on the CLI
    config["catalog"] =  "None";
    config["object-id"] =  "None";

    // Set up a command line parser to accept a subset of the parameters.
    QCommandLineParser parser;
    parser.setApplicationDescription("Camera Configuration Example");
    parser.addHelpOption();

    // Broad configuration options.
    parser.addOption({{"config-file", "f"},     "Path to configuration file", "config-file"});
    parser.addOption({{"camera-config", "cc"},  "Camera configuration name [optional]", "camera-config"});
    parser.addOption({{"exp-config", "ec"},     "Exposure configuration name [optional]", "exp-config"});

    // Camera options
    parser.addOption({"catalog", "Catalog name", "catalog"});
    parser.addOption({"object-id", "Object identifier", "object"});
    parser.addOption({"camera-id", "QHY Camera Identifier", "camera-id"});
    parser.addOption({"filter-names", "List of filters in the camera", ""});
    parser.addOption({"usb-traffic", "QHY USB Traffic Setting", "usb-traffic"});
    parser.addOption({"usb-transferbit", "Bits for image transfer. Options are 8 or 16", "usb-transferbit"});
    parser.addOption({{"camera-bin-mode", "cb"}, "Binning mode. Options: 1x1 - 9x9 further restricted by camera.", "camera-bin-mode"});
    parser.addOption({{"camera-temperature", "ct"}, "Set point for active cooling (Celsius)", "camera-temperature"});
    parser.addOption({{"camera-cool-down", "cool-down", "cd"}, "Instruct the camera to begin cooling to the temperature in `camera-temperature`."});
    parser.addOption({{"camera-warm-up", "warm-up", "cw"}, "Instruct the camera to begin warming up."});

    // exposure options
    parser.addOption({{"exp-quantities", "eq"}, "Number of exposures per filter", "exp-quantities"});
    parser.addOption({{"exp-durations", "ed"},  "Exposure duration, in seconds, per filter", "exp-durations"});
    parser.addOption({{"exp-filters", "ef"},    "Names of filter to use", "exp-filters"});
    parser.addOption({{"exp-gains", "eg"},      "The gain to use per each filter", "exp-gains"});
    parser.addOption({{"exp-offsets", "eo"},    "Image offset per each filter", "exp-offsets"});

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

    // If a specific `camera-config` was specified, replace the values in the
    // default configuration with the values in the sub-configuration
    QString cameraConfig = parser.value("camera-config");
    if(!cameraConfig.isEmpty()) {
        qDebug() << "Loading camera configuration named " << cameraConfig;
        updateDefaultsFromSubConfig(config, cameraConfig);
    }

    // If a specific `camera-config` was specified, replace the values in the
    // default configuration with the values in the sub-configuration
    QString expConfig = parser.value("exp-config");
    if(!expConfig.isEmpty()) {
        qDebug() << "Loading exposure configuration named " << expConfig;
        updateDefaultsFromSubConfig(config, expConfig);
    }

    // Override config values with anything specified on the command line.
    qDebug() << "Updating settings from command line parameters.";
    updateConfigFromCommandLine(config, parser);

    //
    // Verify that the configuration makes sense
    //

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
    } else if(cool_down) {
        config["camera-cool-down"] = "1";
        config["camera-warm-up"]   = "0";
    }

    // Clean up the configuration by removing child configurations
    for(const QString & key: config.keys()) {
        if(key.contains("/")) {
            config.remove(key);
        }
    }

    return config;
}
