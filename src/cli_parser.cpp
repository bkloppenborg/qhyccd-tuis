#include <QSettings>
#include <QtDebug>

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

void printConfig(const QMap<QString, QVariant> & config) {

    // Display final values
    qDebug() << "Configuration file: " << config.value("config-file");
    qDebug() << "Configuration name: " << config.value("config-name");
    qDebug() << "Camera ID: " << config.value("camera-id");
    qDebug() << "Number of exposures: " << config.value("num-exposures");
    qDebug() << "Exposure durations: " << config.value("durations-sec") << " seconds";
    qDebug() << "Filters: " << config.value("filters") << " seconds";
    qDebug() << "Camera gain: " << config.value("gain") ;
    qDebug() << "Catalog: " << config.value("catalog");
    qDebug() << "Object ID: " << config.value("object-id");
}

QMap<QString, QVariant> parse_cli(const QCoreApplication & app) {

    // Configure the QMap with parameters that are relevant to the application
    QMap<QString, QVariant> config;
    config["config-file"] =  "";  // Default is an empty string
    config["config-name"] =  "None";
    config["camera-id"] = "None";
    config["num-exposures"] =  "5";
    config["durations-sec"] =  "10.0";
    config["filters"] =  "None";
    config["gain"] =  1.0;
    config["catalog"] =  "None";
    config["object-id"] =  "None";

    // Set up a command line parser to accept a subset of the parameters.
    QCommandLineParser parser;
    parser.setApplicationDescription("Camera Configuration Example");
    parser.addHelpOption();

    // QMap parameters
    parser.addOption({{"config-file", "f"}, "Path to configuration file", "config-file"});
    parser.addOption({{"config-name", "a"}, "Name of camera configuration block in INI file [default: General]", "config-name"});
    parser.addOption({{"camera-id", "c"}, "Camera ID", "camera-id"});  
    parser.addOption({{"num-exposures", "n"}, "Number of exposures (comma separated)", "num-exposures"});
    parser.addOption({{"durations-sec", "d"}, "Duration of exposures in seconds (comma separated)", "durations-sec"});
    parser.addOption({{"filters", "b"}, "Filters (comma separated)", "filters"});
    parser.addOption({{"gain", "g"}, "Camera gain", "gain",});
    parser.addOption({"catalog", "Catalog name", "catalog"});
    parser.addOption({"object-id", "Object identifier", "object"});
    
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
    }

    // If a specific `config-name` was specified, replace the values in
    // config with those in the sub-configuration.
    const QString configName = parser.value("config-name");
    if(!configName.isEmpty()) {
        QStringList topLevelKeys = getTopLevelKeys(config);
        for(const QString & key: topLevelKeys) {
            const QString subKey = configName + "/" + key;
            if(config.contains(subKey)) {
                config[key] = config[subKey];
            }
        }
    }

    // Override config values with anything specified on the command line.
    qDebug() << "Updating settings from command line parameters.";
    updateConfigFromCommandLine(config, parser);

    // Convert the exposure, durations, and filters into QStringLists
    const QStringList exposures = config["num-exposures"].toString().split(",");
    const QStringList durations = config["durations-sec"].toString().split(",");
    const QStringList filters = config["filters"].toString().split(",");
    config["num-exposures"] = exposures;
    config["durations-sec"] = durations;
    config["filters"] = filters;

    // Verify that exposure, durations, and filters are all the same length.
    if(exposures.length() != durations.length() || 
       exposures.length() != filters.length()) {
        qCritical() << "The number of exposures, exposure durations, or filters do not match.";
        exit(-1);
    }

    // Verify that exposures are integers.
    bool ok = true;
    for(const QString & str: exposures) {
        str.toInt(&ok);
        if(!ok) {
            qCritical() << "num-exposures must be a comma separated list of integer values without any spaces.";
            exit(-1);
        }
    }

    // Verify that exposure durations are numeric.
    for(const QString & str: durations) {
        str.toDouble(&ok);
        if(!ok) {
            qCritical() << "durations-sec must be a comma separated list of numeric values without any spaces.";
            exit(-1);
        }
    }

    return config;
}
