#include "Service.h"
#include "Config.h"
#include <QDebug>
#include <QThread>

namespace Mixer {
Service::Service(QString path, QObject *parent) : QObject(parent), path(std::move(path)) {
    clock.start(); timer.setInterval(40);
    connect(&timer, &QTimer::timeout, this, [this] { tick(); });
    timer.start();
}
void Service::connectDevice() {
    const auto device = discoverHid();
    config = loadConfig(path);
    audio = std::make_unique<Audio>(config);
    audio->ensureRoutes();
    hid = std::make_unique<Hid>(device);
    hid->driverMode(); hid->lights(config);
    changes = ControlChanges{}; faderError.clear();
    try { hid->enableFaders(); changes.reportingEnabled = true; }
    catch (const std::exception &error) { faderError = QString::fromLocal8Bit(error.what()); qWarning().noquote() << faderError; }
    if (changes.reportingEnabled) {
        try {
            QThread::msleep(120); hid->takeReports();
            if (const auto initial = parseControls(hid->input())) changes.update(*initial);
        } catch (const std::exception &error) { qWarning() << "Waiting for initial positions:" << error.what(); }
    }
    lastMutes.reset(); routesAt = 0; refreshAt = 0; lastError.clear();
    qInfo() << "Mixer connected; fader reporting:" << changes.reportingEnabled;
}
void Service::tick() {
    if (stopped) return;
    const auto now = clock.elapsed();
    try {
        if (!hid) { if (now < reconnectAt) return; connectDevice(); }
        if (now >= routesAt) {
            QString configError;
            QJsonObject updated;
            try { updated = loadConfig(path); } catch (const std::exception &error) { configError = error.what(); }
            if (!updated.isEmpty()) {
                if (updated.value("output") != config.value("output") || updated.value("microphone") != config.value("microphone"))
                    fail("Audio device selection changed; reconnecting");
                if (updated.value("color") != config.value("color") || updated.value("muted_color") != config.value("muted_color") || updated.value("brightness") != config.value("brightness")) {
                    hid->lights(updated); lastMutes.reset();
                }
                config = updated; audio->config = updated;
            }
            audio->ensureRoutes(); audio->routeApps();
            publishStatus(true, changes.reportingEnabled, configError.isEmpty() ? faderError : configError);
            routesAt = now + 2000;
        }
        if (now >= refreshAt) {
            audio->refresh(); audio->save();
            const auto muted = audio->muted();
            for (int i = 0; i < 5; ++i) if (!lastMutes || (*lastMutes)[i] != muted[i]) hid->muteFeedback(i + 1, muted[i]);
            lastMutes = muted; refreshAt = now + 500;
        }
        QMap<int, double> volumes;
        for (const auto &raw : hid->takeReports()) {
            const auto report = parseControls(raw); if (!report) continue;
            const auto changed = changes.update(*report);
            if (changes.reportingEnabled) faderError.clear();
            for (auto it = changed.volumes.begin(); it != changed.volumes.end(); ++it) volumes[it.key()] = it.value();
            const int masks[]{1,2,4,8,32};
            for (int i = 0; i < 5; ++i) if (changed.pressed & masks[i]) {
                audio->mute(i, "toggle"); refreshAt = 0;
                qInfo() << "Mute toggled for channel" << i;
            }
        }
        for (auto it = volumes.begin(); it != volumes.end(); ++it) {
            audio->volume(it.key(), it.value());
            qInfo() << "Channel" << it.key() << "volume" << qRound(it.value() * 100);
        }
    } catch (const std::exception &error) {
        const QString message = QString::fromLocal8Bit(error.what());
        if (message != lastError) qWarning().noquote() << message;
        lastError = message; hid.reset();
        try { publishStatus(false, false, message); } catch (const std::exception &) {}
        if (audio) { try { audio->cleanup(); } catch (const std::exception &) {} audio.reset(); }
        reconnectAt = clock.elapsed() + 3000;
    }
}
void Service::stop() {
    if (stopped) return;
    stopped = true; timer.stop(); hid.reset();
    try { publishStatus(false); } catch (const std::exception &) {}
    if (audio) {
        try { audio->refresh(); audio->save(); } catch (const std::exception &) {}
        try { audio->cleanup(); } catch (const std::exception &error) { qWarning() << error.what(); }
    }
}
}
