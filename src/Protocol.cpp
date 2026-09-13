#include "Protocol.h"
#include "Config.h"
#include <algorithm>

namespace Mixer {
QByteArray feature(int commandClass, int command, const QByteArray &args, int size) {
    if (args.size() > 53) fail("Feature arguments exceed 53 bytes");
    QByteArray data(64, 0);
    data[0] = 7; data[2] = 0x1f; data[6] = size < 0 ? args.size() : size;
    data[7] = commandClass; data[8] = command;
    std::copy(args.begin(), args.end(), data.begin() + 9);
    for (int i = 3; i < 62; ++i) data[62] = data.at(62) ^ data.at(i);
    return data;
}
// Exact Windows startup packets. Their captured checksum byte is zero.
QByteArray faderQuery() { return QByteArray::fromHex("070000000000020f950000") + QByteArray(53, 0); }
QByteArray faderEnable() { return QByteArray::fromHex("07001f000000020f150001") + QByteArray(53, 0); }
std::optional<Controls> parseControls(const QByteArray &data) {
    if (data.size() != 8 || data.at(0) != 9) return {};
    Controls result{static_cast<unsigned char>(data.at(1)), {}};
    std::array<int, 4> values{};
    for (int i = 0; i < 4; ++i) values[i] = static_cast<unsigned char>(data.at(i + 3));
    if (std::all_of(values.begin(), values.end(), [](int v) { return v <= 100; })) result.faders = values;
    return result;
}
Changes ControlChanges::update(const Controls &report) {
    Changes changed{report.buttons & ~buttons, {}};
    buttons = report.buttons;
    if (!report.faders) return changed;
    const bool nonzero = std::any_of(report.faders->begin(), report.faders->end(), [](int v) { return v != 0; });
    if (!reportingEnabled && !nonzero) return changed;
    reportingEnabled = true;
    if (faders) for (int i = 0; i < 4; ++i)
        if ((*faders)[i] != (*report.faders)[i]) changed.volumes[i] = (*report.faders)[i] / 100.0;
    faders = report.faders;
    return changed;
}
}
