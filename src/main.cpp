#include <QApplication>
#include <QDebug>
#include <signal.h>

#include <opencv2/highgui.hpp>

#include "cli_parser.hpp"
#include "WorkerThread.hpp"

extern bool keep_running;

void sig_handler(int signal){
    keep_running = false;

    if(signal == SIGINT)
        qDebug() << "Received SIGINT, exiting";
}


int main(int argc, char *argv[]) {

    // Register interrupt handlers
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = sig_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);

    // Configure the application
    QApplication app(argc, argv);
    QApplication::setOrganizationName("Kloppenborg.net");
    QApplication::setOrganizationDomain("kloppenborg.net");
    QApplication::setApplicationName("qhyccd-tuis");

    // Create the worker.
    WorkerThread * worker = new WorkerThread();

    // Create a window, initialize it with an all black background.
    cv::namedWindow("display_window", cv::WINDOW_NORMAL);
    cv::resizeWindow("display_window", 3856*0.3, 2180*0.3);
    cv::Mat temp(2180, 3856, CV_16U);
    cv::imshow("display_window", temp);

    // parse the command line arguments.
    QMap<QString, QVariant> config = parse_cli(app);

    // Configure the worker thread
    app.connect(worker, &WorkerThread::finished, worker, &QObject::deleteLater);
    app.connect(worker, &WorkerThread::finished, &app, &QApplication::quit);
    worker->setConfig(config);
    worker->start();

    return app.exec();
}