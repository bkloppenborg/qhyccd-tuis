#include <QCoreApplication>
#include "cli_parser.hpp"

int main(int argc, char *argv[]) {

    // Configure the application
    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationName("Kloppenborg.net");
    QCoreApplication::setOrganizationDomain("kloppenbor.net");
    QCoreApplication::setApplicationName("qhyccd-tuis");

    QMap<QString, QVariant> config = parse_cli(app);

    printConfig(config);

    return 0;

}