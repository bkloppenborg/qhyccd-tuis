#include <QCoreApplication>
#include <QStringList>
#include <QVariant>
#include <QCommandLineParser>
#include <QMap>

// Function to dump the entire default contents of QSettings to a .ini file
void dumpDefaultConfigToFile(QMap<QString, QVariant> & config, const QString& filePath);

QStringList getTopLevelKeys(QMap<QString, QVariant> config);

void updateConfigFromCommandLine(QMap<QString, QVariant> & config, const QCommandLineParser& parser);

void updateConfigFromFile(QMap<QString, QVariant> & config, const QString& filePath);

void printConfig(const QMap<QString, QVariant> & config);

QMap<QString, QVariant> parse_cli(const QCoreApplication & app);

void checkIntegerType(const QStringList & list, const QString & errorMessage);

void checkNumericType(const QStringList & list, const QString & errorMessage);

QStringList toStringList(const QVariant & var);

void checkMatchingLength(const QStringList & A, const QStringList & B, const QString & errorMessage);