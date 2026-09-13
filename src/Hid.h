#pragma once
#include <QJsonObject>
#include <QList>
#include "Protocol.h"

namespace Mixer {
QString discoverHid();
class Hid {
public:
    explicit Hid(const QString &path);
    ~Hid();
    Hid(const Hid &) = delete;
    Hid &operator=(const Hid &) = delete;
    QList<QByteArray> takeReports();
    QByteArray input();
    QByteArray descriptor();
    void sendFeature(QByteArray data);
    QByteArray transact(const QByteArray &request);
    void driverMode();
    void enableFaders();
    void lights(const QJsonObject &config);
    void muteFeedback(int channel, bool muted);
private:
    int fd = -1;
};
}
