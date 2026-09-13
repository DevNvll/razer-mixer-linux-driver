#include "Controller.h"
#include <QJsonDocument>
#include <QJsonObject>

Controller::Controller(QString executable, QObject *parent) : QObject(parent), executable(std::move(executable)) {
    pollTimeout.setSingleShot(true); actionTimeout.setSingleShot(true);
    connect(&pollTimeout, &QTimer::timeout, this, [this] { loadError = "Reading mixer status timed out"; poll.kill(); emit errorChanged(); });
    connect(&actionTimeout, &QTimer::timeout, this, [this] { actionError = "Mixer command timed out"; action.kill(); emit errorChanged(); });
    connect(&poll, &QProcess::finished, this, [this](int code, QProcess::ExitStatus exit) {
        pollTimeout.stop();
        const auto doc = QJsonDocument::fromJson(poll.readAllStandardOutput());
        if (code == 0 && exit == QProcess::NormalExit && doc.isObject()) {
            state = doc.object().toVariantMap(); loadError.clear();
        } else {
            loadError = QString::fromUtf8(poll.readAllStandardError()).trimmed();
            if (loadError.isEmpty()) loadError = "Cannot read mixer status";
            auto device = state.value("device").toMap(); device["connected"] = false; device["fadersReady"] = false; state["device"] = device;
        }
        emit snapshotChanged(); emit errorChanged();
    });
    connect(&poll, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) { pollTimeout.stop(); loadError = "Cannot start the mixer driver. Check its installation."; emit errorChanged(); }
    });
    connect(&action, &QProcess::finished, this, [this](int code, QProcess::ExitStatus exit) {
        actionTimeout.stop();
        actionError = code == 0 && exit == QProcess::NormalExit ? QString{} : QString::fromUtf8(action.readAllStandardError()).trimmed();
        if ((code != 0 || exit != QProcess::NormalExit) && actionError.isEmpty()) actionError = "Mixer command failed";
        emit errorChanged(); emit busyChanged(); dispatch(); refresh();
    });
    connect(&action, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            actionTimeout.stop(); actionError = "Cannot start the mixer driver. Check its installation.";
            pending.clear(); emit errorChanged(); emit busyChanged();
        }
    });
    pollTimer.setInterval(600); connect(&pollTimer, &QTimer::timeout, this, &Controller::refresh);
    pollTimer.start(); QTimer::singleShot(0, this, &Controller::refresh);
}
Controller::~Controller() {
    pollTimer.stop(); pollTimeout.stop(); actionTimeout.stop();
    poll.disconnect(this); action.disconnect(this);
    for (auto process : {&poll, &action}) if (process->state() != QProcess::NotRunning) { process->kill(); process->waitForFinished(1000); }
}
QString Controller::error() const {
    if (!actionError.isEmpty()) return actionError;
    if (!loadError.isEmpty()) return loadError;
    return state.value("device").toMap().value("error").toString();
}
void Controller::refresh() {
    if (poll.state() != QProcess::NotRunning || busy()) return;
    poll.start(executable, {"status"}); pollTimeout.start(8000);
}
void Controller::enqueue(QStringList args) {
    actionError.clear(); emit errorChanged();
    if (args[0] == "lighting") {
        for (auto it = pending.begin(); it != pending.end();) {
            if (it->size() > 1 && (*it)[0] == args[0] && (*it)[1] == args[1]) it = pending.erase(it);
            else ++it;
        }
    }
    pending.append(std::move(args)); dispatch(); emit busyChanged();
}
void Controller::dispatch() {
    if (action.state() != QProcess::NotRunning || pending.isEmpty()) return;
    action.start(executable, pending.takeFirst()); actionTimeout.start(8000);
}
void Controller::setMute(int channel, bool muted) { if (channel >= 0 && channel <= 3) enqueue({"mute", QString::number(channel), muted ? "1" : "0"}); }
void Controller::setLighting(const QString &key, const QString &value) { enqueue({"lighting", key, value}); }
void Controller::route(const QString &application, const QString &channel) { enqueue({"route", application, channel}); }
void Controller::initialize() { enqueue({"init"}); }
void Controller::startDriver() { enqueue({"start"}); }
