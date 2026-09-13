#pragma once
#include <QByteArray>
#include <QMap>
#include <array>
#include <optional>

namespace Mixer {
QByteArray feature(int commandClass, int command, const QByteArray &args, int size = -1);
QByteArray faderQuery();
QByteArray faderEnable();
struct Controls { int buttons; std::optional<std::array<int, 4>> faders; };
std::optional<Controls> parseControls(const QByteArray &data);
struct Changes { int pressed; QMap<int, double> volumes; };
class ControlChanges {
public:
    bool reportingEnabled = false;
    std::optional<std::array<int, 4>> faders;
    Changes update(const Controls &report);
private:
    int buttons = 0;
};
}
