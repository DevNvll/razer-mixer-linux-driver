#include "Control.h"
#include "Audio.h"
#include <QDateTime>
#include <QFile>
#include <algorithm>

namespace Mixer {
QJsonObject status(const QString &path) {
    const bool configured = QFile::exists(path);
    const auto config = configured ? loadConfig(path) : defaults();
    QJsonObject device;
    try { device = readJson(runtimePath() + "/razer-mixer-status.json"); } catch (const std::exception &) {}
    const bool fresh = QDateTime::currentMSecsSinceEpoch() / 1000.0 - device.value("updated").toDouble() < 10;
    const bool connected = fresh && device.value("connected").toBool();
    device["connected"] = connected;
    device["fadersReady"] = connected && device.value("fadersReady").toBool();
    try { device["service"] = run("systemctl", {"--user", "show", "razer-mixer-bridge.service", "--property=ActiveState", "--value"}); }
    catch (const std::exception &) { device["service"] = "unavailable"; }
    QHash<QString, QJsonObject> nodes, clients;
    QHash<int, QString> sinks;
    for (const auto &kind : {"sinks", "sources"}) for (const auto &value : listing(kind)) {
        const auto node = value.toObject(); nodes[node.value("name").toString()] = node;
        if (QString(kind) == "sinks") sinks[node.value("index").toInt()] = node.value("name").toString();
    }
    QJsonArray channelRows;
    const QStringList names{config.value("output").toString(), "razer_mixer_chat", "razer_mixer_music", config.value("microphone").toString()};
    const QStringList labels{"Master", "Chat", "Music", "Mic"};
    for (int i = 0; i < 4; ++i) {
        const auto node = nodes.value(names[i]);
        channelRows.append(QJsonObject{{"id", i}, {"name", names[i]}, {"label", labels[i]}, {"available", !node.isEmpty()},
                           {"volume", nodeVolume(node)}, {"muted", node.value("mute").toBool()}});
    }
    QHash<QString, QJsonObject> apps{{"discord", {{"id", "discord"}, {"name", "Discord"}, {"running", false}}},
                                    {"spotify", {{"id", "spotify"}, {"name", "Spotify"}, {"running", false}}}};
    const auto routes = config.value("routes").toObject();
    for (auto it = routes.begin(); it != routes.end(); ++it) if (!apps.contains(it.key()))
        apps[it.key()] = {{"id", it.key()}, {"name", it.key()}, {"running", false}};
    for (const auto &value : listing("clients")) { const auto client = value.toObject(); clients[client.value("index").toVariant().toString()] = client.value("properties").toObject(); }
    for (const auto &value : listing("sink-inputs")) {
        const auto stream = value.toObject();
        if (!names.mid(0, 3).contains(sinks.value(stream.value("sink").toInt()))) continue;
        auto props = clients.value(stream.value("client").toVariant().toString());
        const auto own = stream.value("properties").toObject();
        for (auto it = own.begin(); it != own.end(); ++it) props[it.key()] = it.value();
        if (props.value("application.id") == "razer-mixer-bridge") continue;
        QString key;
        for (const auto &field : {"application.process.binary", "application.id", "application.name"})
            if (key.isEmpty()) key = props.value(field).toString().toLower();
        if (key.isEmpty()) continue;
        apps[key] = {{"id", key}, {"name", props.value("application.name").toString(key)}, {"running", true}, {"properties", props}};
        if (key == "spotify") apps[key]["name"] = "Spotify";
        if (key == "discord") apps[key]["name"] = "Discord";
    }
    auto sorted = apps.values();
    std::sort(sorted.begin(), sorted.end(), [](const QJsonObject &a, const QJsonObject &b) { return a.value("name").toString().toLower() < b.value("name").toString().toLower(); });
    QJsonArray appRows;
    for (auto app : sorted) {
        const auto props = app.contains("properties") ? app.take("properties").toObject() : QJsonObject{{"application.process.binary", app.value("id")}};
        const auto target = applicationChannel(props, config);
        app["channel"] = target == "razer_mixer_chat" ? "chat" : target == "razer_mixer_music" ? "music" : "master";
        appRows.append(app);
    }
    return {{"configured", configured}, {"config", config}, {"device", device}, {"channels", channelRows}, {"apps", appRows}};
}
QJsonObject execute(const QStringList &args, const QString &path) {
    if (args.isEmpty()) fail("Missing command");
    const auto command = args[0];
    if (command == "status") return status(path);
    if (command == "init") {
        if (!QFile::exists(path)) writeJson(path, defaults());
        return loadConfig(path);
    }
    if (command == "start") {
        run("systemctl", {"--user", "start", "razer-mixer-bridge.service"});
        return {{"started", true}};
    }
    if (command == "lighting" && args.size() == 3) {
        const auto key = args[1] == "muted-color" ? "muted_color" : args[1];
        if (!QStringList{"color", "muted_color", "brightness"}.contains(key)) fail("Unknown lighting setting");
        return editConfig([&](QJsonObject &config) {
            if (key == "brightness") { bool ok; const int value = args[2].toInt(&ok); if (!ok) fail("Invalid brightness"); config[key] = value; }
            else config[key] = args[2];
        }, path);
    }
    if (command == "route" && args.size() == 3) return editConfig([&](QJsonObject &config) {
        auto routes = config.value("routes").toObject(); routes[args[1].toLower()] = args[2]; config["routes"] = routes;
    }, path);
    if ((command == "volume" || command == "mute") && args.size() == 3) {
        bool ok; const int index = args[1].toInt(&ok);
        if (!ok || index < 0 || index > 3) fail("Channel must be 0 through 3");
        Audio audio(loadConfig(path)); audio.refresh();
        if (command == "volume") { const double volume = args[2].toDouble(&ok); if (!ok) fail("Invalid volume"); audio.volume(index, volume); }
        else audio.mute(index, args[2]);
        return {{"saved", true}};
    }
    fail("Unknown command or arguments");
}
}
