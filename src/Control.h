#pragma once
#include "Config.h"
namespace Mixer {
QJsonObject status(const QString &path = configPath());
QJsonObject execute(const QStringList &arguments, const QString &path = configPath());
}
