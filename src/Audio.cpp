#include "Audio.h"
#include "Config.h"
#include <QJsonDocument>
#include <QRegularExpression>
#include <algorithm>
#include <cmath>

namespace Mixer {
static const QStringList channels{"razer_mixer_chat", "razer_mixer_music"};
QJsonArray listing(const QString &kind) {
    if (kind == "modules") {
        QJsonArray result;
        for (const auto &line : run("pactl", {"list", "short", "modules"}).split('\n')) {
            const auto parts = line.split('\t');
            if (parts.size() >= 3) result.append(QJsonObject{{"index", parts[0]}, {"name", parts[1]}, {"argument", parts[2]}});
        }
        return result;
    }
    const auto doc = QJsonDocument::fromJson(run("pactl", {"-f", "json", "list", kind}).toUtf8());
    if (!doc.isArray()) fail("Invalid audio server response for " + kind);
    return doc.array();
}
QString moduleArgument(const QString &arguments, const QString &key) {
    const QRegularExpression pattern("(?:^|\\s)" + QRegularExpression::escape(key) + "=(\"[^\"]*\"|'[^']*'|\\S+)");
    auto value = pattern.match(arguments).captured(1);
    if (value.startsWith('"') || value.startsWith('\'')) value = value.mid(1, value.size() - 2);
    return value;
}
QString applicationChannel(const QJsonObject &properties, const QJsonObject &config) {
    const QStringList keys{"application.process.binary", "application.id", "application.name"};
    const auto routes = config.value("routes").toObject();
    for (const auto &key : keys) {
        const auto route = routes.value(properties.value(key).toString().toLower()).toString();
        if (!route.isEmpty()) return route == "master" ? config.value("output").toString() : "razer_mixer_" + route;
    }
    const auto apps = config.value("apps").toObject();
    for (auto it = apps.begin(); it != apps.end(); ++it)
        for (const auto &app : it.value().toArray())
            for (const auto &key : keys)
                if (app.toString().compare(properties.value(key).toString(), Qt::CaseInsensitive) == 0)
                    return "razer_mixer_" + it.key();
    return {};
}
double nodeVolume(const QJsonObject &node) {
    double volume = 0;
    const auto values = node.value("volume").toObject();
    for (const auto &value : values) volume = std::max(volume, value.toObject().value("value").toDouble() / 65536.0);
    return volume;
}
Audio::Audio(QJsonObject initial) : config(std::move(initial)) {
    try { saved = readJson(statePath() + "/channels.json"); } catch (const std::exception &) {}
}
void Audio::refresh() {
    sinks.clear(); sources.clear();
    for (const auto &value : listing("sinks")) { const auto node = value.toObject(); sinks[node.value("name").toString()] = node; }
    for (const auto &value : listing("sources")) { const auto node = value.toObject(); sources[node.value("name").toString()] = node; }
    if (!sinks.contains(config.value("output").toString()) || !sources.contains(config.value("microphone").toString()))
        fail("Waiting for the configured mixer output and microphone");
}
void Audio::ensureRoutes() {
    refresh();
    const auto modules = listing("modules");
    for (const auto &name : channels) {
        if (!sinks.contains(name)) {
            const auto description = name.endsWith("chat") ? "Razer Chat" : "Razer Music";
            run("pactl", {"load-module", "module-null-sink", "sink_name=" + name,
                          QString("sink_properties=\"device.description='%1'\"").arg(description),
                          "rate=48000", "channels=2", "channel_map=front-left,front-right"});
            const auto state = saved.value(name).toObject();
            run("pactl", {"set-sink-volume", name, QString::number(state.value("volume").toInt(65536))});
            run("pactl", {"set-sink-mute", name, state.value("mute").toBool() ? "1" : "0"});
        }
        bool exists = false;
        for (const auto &value : modules) {
            const auto module = value.toObject();
            const auto args = module.value("argument").toString();
            if (module.value("name") == "module-loopback" && moduleArgument(args, "source") == name + ".monitor"
                && moduleArgument(args, "sink") == config.value("output").toString()) exists = true;
        }
        if (!exists) run("pactl", {"load-module", "module-loopback", "source=" + name + ".monitor",
                                  "sink=" + config.value("output").toString(), "latency_msec=20", "source_dont_move=true",
                                  "sink_dont_move=true", "sink_input_properties=application.id=razer-mixer-bridge"});
    }
    refresh();
}
void Audio::routeApps() {
    QHash<QString, QJsonObject> clients;
    for (const auto &value : listing("clients")) {
        const auto client = value.toObject(); clients[client.value("index").toVariant().toString()] = client.value("properties").toObject();
    }
    for (const auto &value : listing("sink-inputs")) {
        const auto stream = value.toObject();
        auto props = clients.value(stream.value("client").toVariant().toString());
        const auto streamProps = stream.value("properties").toObject();
        for (auto it = streamProps.begin(); it != streamProps.end(); ++it) props[it.key()] = it.value();
        if (props.value("application.id") == "razer-mixer-bridge") continue;
        const auto target = applicationChannel(props, config);
        if (sinks.contains(target) && stream.value("sink") != sinks[target].value("index"))
            run("pactl", {"move-sink-input", stream.value("index").toVariant().toString(), target});
    }
}
QJsonObject Audio::target(int channel) const {
    if (channel < 0 || channel > 4) fail("Channel must be 0 through 3, or 4 for the dedicated mic button");
    const auto name = channel >= 3 ? config.value("microphone").toString()
                     : channel == 0 ? config.value("output").toString() : channels[channel - 1];
    const auto node = channel >= 3 ? sources.value(name) : sinks.value(name);
    if (node.isEmpty()) fail("Audio channel is unavailable");
    return node;
}
void Audio::volume(int channel, double value) {
    if (!std::isfinite(value)) fail("Volume must be a finite number");
    const auto node = target(channel);
    run("wpctl", {"set-volume", node.value("properties").toObject().value("object.id").toVariant().toString(),
                  QString::number(std::clamp(value, 0.0, 1.0), 'f', 4)});
}
void Audio::mute(int channel, const QString &value) {
    if (!QStringList{"0", "1", "toggle"}.contains(value)) fail("Mute must be 0, 1 or toggle");
    run("pactl", {channel >= 3 ? "set-source-mute" : "set-sink-mute", target(channel).value("name").toString(), value});
}
std::array<bool, 5> Audio::muted() const {
    std::array<bool, 5> result{};
    for (int i = 0; i < 5; ++i) result[i] = target(i).value("mute").toBool();
    return result;
}
void Audio::save() {
    QJsonObject state;
    for (const auto &name : channels) if (sinks.contains(name))
        state[name] = QJsonObject{{"volume", int(std::lround(nodeVolume(sinks[name]) * 65536))}, {"mute", sinks[name].value("mute")}};
    if (state == saved) return;
    writeJson(statePath() + "/channels.json", state); saved = state;
}
void Audio::cleanup() {
    QHash<QString, int> current;
    for (const auto &value : listing("sinks")) { const auto node = value.toObject(); current[node.value("name").toString()] = node.value("index").toInt(); }
    auto fallback = config.value("output").toString();
    if (!current.contains(fallback)) fallback = run("pactl", {"get-default-sink"});
    if (!channels.contains(fallback)) for (const auto &value : listing("sink-inputs")) {
        const auto stream = value.toObject();
        for (const auto &name : channels) if (current.contains(name) && stream.value("sink").toInt() == current[name])
            run("pactl", {"move-sink-input", stream.value("index").toVariant().toString(), fallback});
    }
    const auto modules = listing("modules");
    for (const auto &kind : {"module-loopback", "module-null-sink"}) for (const auto &value : modules) {
        const auto module = value.toObject(); if (module.value("name") != kind) continue;
        const auto args = module.value("argument").toString();
        const bool ours = QString(kind) == "module-null-sink" ? channels.contains(moduleArgument(args, "sink_name"))
                          : QStringList{"razer_mixer_chat.monitor", "razer_mixer_music.monitor"}.contains(moduleArgument(args, "source"))
                            && moduleArgument(args, "sink") == config.value("output").toString();
        if (ours) run("pactl", {"unload-module", module.value("index").toVariant().toString()});
    }
}
}
