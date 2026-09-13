#include "Config.h"
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QSaveFile>
#include <cmath>
#include <stdexcept>
#include <sys/file.h>
#include <unistd.h>

namespace Mixer {
void fail(const QString &message) { throw std::runtime_error(message.toStdString()); }
QString run(const QString &program, const QStringList &arguments, int timeout) {
    QProcess process;
    auto env = QProcessEnvironment::systemEnvironment();
    env.insert("LC_ALL", "C.UTF-8");
    process.setProcessEnvironment(env);
    process.start(program, arguments);
    if (!process.waitForStarted(1000)) fail("Cannot start " + program + ": " + process.errorString());
    if (!process.waitForFinished(timeout)) {
        process.kill();
        process.waitForFinished();
        fail(program + " timed out");
    }
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0)
        fail(program + ": " + QString::fromUtf8(process.readAllStandardError()).trimmed());
    return QString::fromUtf8(process.readAllStandardOutput()).trimmed();
}
QString configPath() { return qEnvironmentVariable("XDG_CONFIG_HOME", QDir::homePath() + "/.config") + "/razer-mixer/config.json"; }
QString statePath() { return qEnvironmentVariable("XDG_STATE_HOME", QDir::homePath() + "/.local/state") + "/razer-mixer"; }
QString runtimePath() { return qEnvironmentVariable("XDG_RUNTIME_DIR", "/run/user/" + QString::number(getuid())); }
QJsonObject defaults() {
    return {{"output", "alsa_output.usb-Razer_Razer_Audio_Mixer-00.analog-stereo"},
            {"microphone", "alsa_input.usb-Razer_Razer_Audio_Mixer-00.analog-stereo"},
            {"apps", QJsonObject{{"chat", QJsonArray{"discord", "vesktop", "dev.vencord.Vesktop", "com.discordapp.Discord"}},
                                 {"music", QJsonArray{"spotify", "com.spotify.Client"}}}},
            {"color", "00ff40"}, {"muted_color", "ff0000"}, {"brightness", 50}, {"routes", QJsonObject{}}};
}
QJsonObject validate(QJsonObject config) {
    static const QRegularExpression rgb("^#?[0-9a-fA-F]{6}$");
    for (const auto &key : {"color", "muted_color"}) {
        if (config.contains(key) && !config.value(key).isString()) fail(QString(key) + " must be a six-digit hex color");
        auto value = config.value(key).toString(defaults().value(key).toString());
        if (!rgb.match(value).hasMatch()) fail(QString(key) + " must be a six-digit hex color");
        config[key] = value.remove('#').toLower();
    }
    const auto brightness = config.value("brightness").isUndefined() ? QJsonValue(50) : config.value("brightness");
    if (!brightness.isDouble() || brightness.toDouble() < 0 || brightness.toDouble() > 100 || std::floor(brightness.toDouble()) != brightness.toDouble())
        fail("Brightness must be an integer from 0 to 100");
    config["brightness"] = brightness;
    for (const auto &key : {"output", "microphone"})
        if (config.value(key).toString().isEmpty()) fail(QString("Missing ") + key);
    if (!config.value("apps").isObject()) fail("Apps must contain Chat and Music application lists");
    const auto apps = config.value("apps").toObject();
    for (auto it = apps.begin(); it != apps.end(); ++it) {
        if ((it.key() != "chat" && it.key() != "music") || !it.value().isArray()) fail("Invalid application list");
        for (const auto &value : it.value().toArray()) if (!value.isString()) fail("Application names must be strings");
    }
    if (config.contains("routes") && !config.value("routes").isObject()) fail("Routes must be an object");
    const auto routes = config.value("routes").toObject();
    for (auto it = routes.begin(); it != routes.end(); ++it)
        if (it.key().isEmpty() || !QStringList{"master", "chat", "music"}.contains(it.value().toString())) fail("Invalid application route");
    return config;
}
QJsonObject readJson(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) fail("Cannot read " + path + ": " + file.errorString());
    QJsonParseError error;
    const auto doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) fail("Invalid JSON object in " + path);
    return doc.object();
}
QJsonObject loadConfig(const QString &path) { return validate(readJson(path)); }
void writeJson(const QString &path, const QJsonObject &value) {
    QDir().mkpath(QFileInfo(path).absolutePath());
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) fail("Cannot write " + path);
    const auto bytes = QJsonDocument(value).toJson();
    if (file.write(bytes) != bytes.size() || !file.commit()) fail("Cannot save " + path);
}
QJsonObject editConfig(const std::function<void(QJsonObject &)> &edit, const QString &path) {
    QDir().mkpath(QFileInfo(path).absolutePath());
    const auto info = QFileInfo(path);
    QFile lock(info.absolutePath() + "/" + info.completeBaseName() + ".lock");
    if (!lock.open(QIODevice::ReadWrite) || flock(lock.handle(), LOCK_EX) != 0) fail("Cannot lock mixer settings");
    auto config = loadConfig(path);
    edit(config);
    config = validate(config);
    writeJson(path, config);
    return config;
}
void publishStatus(bool connected, bool fadersReady, const QString &error) {
    writeJson(runtimePath() + "/razer-mixer-status.json", {{"connected", connected}, {"fadersReady", fadersReady},
              {"error", error}, {"updated", QDateTime::currentMSecsSinceEpoch() / 1000.0}});
}
}
