#pragma once
#include <QJsonObject>
#include <QStringList>
#include <functional>

namespace Mixer {
[[noreturn]] void fail(const QString &message);
QString run(const QString &program, const QStringList &arguments, int timeout = 5000);
QString configPath();
QString statePath();
QString runtimePath();
QJsonObject defaults();
QJsonObject validate(QJsonObject config);
QJsonObject readJson(const QString &path);
QJsonObject loadConfig(const QString &path = configPath());
void writeJson(const QString &path, const QJsonObject &value);
QJsonObject editConfig(const std::function<void(QJsonObject &)> &edit, const QString &path = configPath());
void publishStatus(bool connected, bool fadersReady = false, const QString &error = {});
}
