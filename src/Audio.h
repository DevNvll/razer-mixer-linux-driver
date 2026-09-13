#pragma once
#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <array>

namespace Mixer {
QJsonArray listing(const QString &kind);
QString applicationChannel(const QJsonObject &properties, const QJsonObject &config);
QString moduleArgument(const QString &arguments, const QString &key);
double nodeVolume(const QJsonObject &node);
class Audio {
public:
    explicit Audio(QJsonObject config);
    QJsonObject config;
    QHash<QString, QJsonObject> sinks, sources;
    void refresh();
    void ensureRoutes();
    void routeApps();
    void volume(int channel, double value);
    void mute(int channel, const QString &value);
    std::array<bool, 5> muted() const;
    void save();
    void cleanup();
    QJsonObject target(int channel) const;
private:
    QJsonObject saved;
};
}
