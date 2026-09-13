#include "Control.h"
#include "Service.h"
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QTextStream>
#include <QThread>
#include <csignal>
#include <sys/file.h>

static volatile std::sig_atomic_t stopping = 0;
static void requestStop(int) { stopping = 1; }
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    app.setApplicationName("razer-mixer-driver"); app.setApplicationVersion("0.1.0");
    QCommandLineParser parser;
    parser.setApplicationDescription("Razer Audio Mixer driver and protocol tools");
    parser.addHelpOption(); parser.addVersionOption();
    parser.addOption({"config", "Settings file", "path", Mixer::configPath()});
    parser.addOption({"duration", "Monitor duration in seconds", "seconds", "30"});
    parser.addPositionalArgument("command", "run, init, status, volume, mute, lighting, route, start, devices, descriptor, monitor");
    parser.addPositionalArgument("arguments", "Command arguments", "[arguments...]");
    parser.process(app);
    auto args = parser.positionalArguments(); if (args.isEmpty()) args.append("run");
    try {
        if (args[0] == "run") {
            QFile lock(Mixer::runtimePath() + "/razer-mixer-bridge.lock");
            if (!lock.open(QIODevice::ReadWrite) || flock(lock.handle(), LOCK_EX | LOCK_NB) != 0) Mixer::fail("A mixer driver is already running");
            std::signal(SIGINT, requestStop); std::signal(SIGTERM, requestStop);
            Mixer::Service service(parser.value("config"));
            QObject::connect(&app, &QCoreApplication::aboutToQuit, &service, [&service] { service.stop(); });
            QTimer signalTimer; QObject::connect(&signalTimer, &QTimer::timeout, &app, [&app] { if (stopping) app.quit(); }); signalTimer.start(100);
            return app.exec();
        }
        if (args[0] == "devices") {
            QTextStream(stdout) << QJsonDocument(QJsonObject{{"sinks", Mixer::listing("sinks")}, {"sources", Mixer::listing("sources")}}).toJson();
        } else if (args[0] == "descriptor" || args[0] == "monitor") {
            Mixer::Hid device(Mixer::discoverHid());
            if (args[0] == "descriptor") QTextStream(stdout) << device.descriptor().toHex(' ') << '\n';
            else {
                bool ok; const int seconds = parser.value("duration").toInt(&ok);
                if (!ok || seconds < 1) Mixer::fail("Duration must be a positive number of seconds");
                QElapsedTimer clock; clock.start();
                while (clock.elapsed() < qint64(seconds) * 1000) {
                    for (const auto &data : device.takeReports())
                        QTextStream(stdout) << QJsonDocument(QJsonObject{{"elapsed_ms", clock.elapsed()}, {"hex", QString(data.toHex(' '))}}).toJson(QJsonDocument::Compact) << Qt::endl;
                    QThread::msleep(5);
                }
            }
        } else QTextStream(stdout) << QJsonDocument(Mixer::execute(args, parser.value("config"))).toJson(QJsonDocument::Compact) << '\n';
        return 0;
    } catch (const std::exception &error) { QTextStream(stderr) << error.what() << '\n'; return 1; }
}
