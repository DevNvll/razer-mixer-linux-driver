#include "Hid.h"
#include "Config.h"
#include <QDir>
#include <QFile>
#include <QThread>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <linux/hidraw.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace Mixer {
static void ioError() { fail(QString::fromLocal8Bit(std::strerror(errno))); }
QString discoverHid() {
    QStringList matches;
    for (const auto &name : QDir("/sys/class/hidraw").entryList({"hidraw*"}, QDir::Dirs)) {
        QFile info("/sys/class/hidraw/" + name + "/device/uevent");
        if (!info.open(QIODevice::ReadOnly)) continue;
        for (const auto &line : info.readAll().split('\n')) {
            if (!line.startsWith("HID_ID=")) continue;
            const auto parts = line.mid(7).split(':');
            if (parts.size() == 3 && parts[1].toUInt(nullptr, 16) == 0x1532 && parts[2].toUInt(nullptr, 16) == 0x053e)
                matches.append("/dev/" + name);
        }
    }
    if (matches.isEmpty()) fail("Connect the Razer Audio Mixer");
    if (matches.size() != 1) fail("More than one Razer Audio Mixer is connected");
    return matches.first();
}
Hid::Hid(const QString &path) {
    fd = ::open(QFile::encodeName(path).constData(), O_RDWR | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) ioError();
}
Hid::~Hid() { if (fd >= 0) ::close(fd); }
void Hid::sendFeature(QByteArray data) {
    if (data.size() != 64) fail("Expected a 64-byte feature report");
    if (ioctl(fd, HIDIOCSFEATURE(64), data.data()) < 0) ioError();
}
QByteArray Hid::transact(const QByteArray &request) {
    sendFeature(request);
    for (int attempt = 0; attempt < 5; ++attempt) {
        QThread::msleep(10);
        QByteArray response(64, 0); response[0] = 7;
        if (ioctl(fd, HIDIOCGFEATURE(64), response.data()) < 0) ioError();
        if (response.at(0) != 7 || response.mid(7, 2) != request.mid(7, 2)) continue;
        if (response.at(1) == 2) return response;
        if (response.at(1) != 1) fail("Mixer rejected command " + request.mid(7, 2).toHex());
    }
    fail("No acknowledgement for mixer command " + request.mid(7, 2).toHex());
}
QList<QByteArray> Hid::takeReports() {
    QList<QByteArray> reports;
    for (;;) {
        QByteArray data(256, 0);
        const auto size = ::read(fd, data.data(), data.size());
        if (size < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) break;
        if (size < 0) ioError();
        if (size == 0) fail("Mixer disconnected");
        data.resize(size); reports.append(data);
    }
    return reports;
}
QByteArray Hid::input() {
    QByteArray data(16, 0); data[0] = 9;
    if (ioctl(fd, HIDIOCGINPUT(16), data.data()) < 0) ioError();
    return data.left(8);
}
QByteArray Hid::descriptor() {
    int size = 0;
    if (ioctl(fd, HIDIOCGRDESCSIZE, &size) < 0) ioError();
    if (size < 1 || size > HID_MAX_DESCRIPTOR_SIZE) fail("Invalid HID descriptor length");
    hidraw_report_descriptor data{}; data.size = size;
    if (ioctl(fd, HIDIOCGRDESC, &data) < 0) ioError();
    return QByteArray(reinterpret_cast<const char *>(data.value), size);
}
void Hid::driverMode() { transact(feature(0, 4, QByteArray::fromHex("0300"))); }
void Hid::enableFaders() {
    auto reply = transact(faderQuery());
    if (reply.mid(9, 2) == QByteArray::fromHex("0001")) return;
    if (reply.mid(9, 2) != QByteArray::fromHex("0000")) fail("Unexpected fader reporting state");
    sendFeature(faderEnable());
    QThread::msleep(20);
    if (transact(faderQuery()).mid(9, 2) != QByteArray::fromHex("0001")) fail("Fader reporting did not enable");
}
void Hid::muteFeedback(int channel, bool muted) {
    if (channel < 1 || channel > 5) fail("Invalid mute channel");
    QByteArray args; args.append(char(0)).append(char(channel)).append(char(muted));
    transact(feature(8, 0x10, args));
}
void Hid::lights(const QJsonObject &config) {
    const auto rgb = QByteArray::fromHex(config.value("color").toString().toLatin1());
    const auto muted = QByteArray::fromHex(config.value("muted_color").toString().toLatin1());
    const int brightness = config.value("brightness").toInt();
    const int zones[][3] = {{4,1,0x1b},{5,4,0x24},{6,1,0x1b},{7,1,0x1b},{8,1,0x1b},{9,1,0x1b},{10,4,0x24},{16,8,0x1b},{32,2,0x1e},{33,2,0x1e}};
    for (const auto &zone : zones) {
        QByteArray level; level.append(char(1)).append(char(zone[0])).append(char(brightness));
        transact(feature(15, 4, level));
        auto colors = rgb.repeated(zone[1]);
        if (zone[0] == 16) colors = (rgb + muted).repeated(4);
        if (zone[0] == 32 || zone[0] == 33) colors = rgb + muted;
        QByteArray args; args.append(char(0)).append(char(zone[0])).append(char(1)).append(char(0)).append(char(0)).append(char(zone[1]));
        transact(feature(15, 2, args + colors, zone[2]));
    }
}
}
