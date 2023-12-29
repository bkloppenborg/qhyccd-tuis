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

    if(var.type() == QVariant::Type::String) {
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

    // Configuration options typically specified in a exposure configuration block
    config["exp-quantities"] = "5";
    config["exp-durations"] = "10";
    config["exp-filters"] = "None";
    config["exp-gains"] = "1.0";    // typically doesn't change between exposures, automatically replicated if needed.
    config["exp-offsets"] =  "100";  // typically doesn't change between exposures, automatically replicated if needed.

    // Configuration typically specified on the CLI
    config["catalog"] =  "None";
    config["object-id"] =  "None";

    // Set up a command line parser to accept a subset of the parameters.
    QCommandLineParser parser;
    parser.setApplicationDescription("Camera Configuration Example");
    parser.addHelpOption();

    // QMap parameters
    parser.addOption({{"config-file", "f"},     "Path to configuration file", "config-file"});
    parser.addOption({{"camera-config", "cc"},  "Camera configuration name [optional]", "camera-config"});
    parser.addOption({{"exp-config", "ec"},     "Exposure configuration name [optional]", "exp-config"});

    parser.addOption({{"exp-quantities", "eq"}, "Number of exposures per filter", "exp-quantities"});
    parser.addOption({{"exp-durations", "ed"},  "Exposure duration, in seconds, per filter", "exp-durations"});
    parser.addOption({{"exp-filters", "ef"},    "Names of filter to use", "exp-filters"});
    parser.addOption({{"exp-gains", "eg"},      "The gain to use per each filter", "exp-gains"});
    parser.addOption({{"exp-offsets", "eo"},    "Image offset per each filter", "exp-offsets"});

    parser.addOption({"catalog", "Catalog name", "catalog"});
    parser.addOption({"object-id", "Object identifier", "object"});
    parser.addOption({"camera-id", "QHY Camera Identifier", "camera-id"});
    parser.addOption({"filter-names", "List of filters in the camera", ""});
    parser.addOption({"usb-traffic", "QHY USB Traffic Setting", "usb-traffic"});
    parser.addOption({"usb-transferbit", "Bits for image transfer. Options are 8 or 16", "usb-transferbit"});
    
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
    const QStringList filters = toStringList(config["exp-filters"]);
    checkMatchingLength(quantities, filters, "The number of filters does match the number of exposures");
    config["filters"] = filters;

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

    // Clean up the configuration by removing child configurations
    for(const QString & key: config.keys()) {
        if(key.contains("/")) {
            config.remove(key);
        }
    }

    return config;
}