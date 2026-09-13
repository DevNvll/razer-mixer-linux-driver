#pragma once
#include "Audio.h"
#include "Hid.h"
#include <QElapsedTimer>
#include <QObject>
#include <QTimer>
#include <memory>

namespace Mixer {
class Service : public QObject {
public:
    explicit Service(QString path, QObject *parent = nullptr);
    void stop();
private:
    void tick();
    void connectDevice();
    QString path, lastError, faderError;
    QJsonObject config;
    std::unique_ptr<Audio> audio;
    std::unique_ptr<Hid> hid;
    ControlChanges changes;
    std::optional<std::array<bool, 5>> lastMutes;
    QElapsedTimer clock;
    QTimer timer;
    qint64 reconnectAt = 0, routesAt = 0, refreshAt = 0;
    bool stopped = false;
};
}
