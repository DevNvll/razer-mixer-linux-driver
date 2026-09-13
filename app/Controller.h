#pragma once
#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QVariantMap>

class Controller : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantMap snapshot READ snapshot NOTIFY snapshotChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
public:
    explicit Controller(QString executable, QObject *parent = nullptr);
    ~Controller() override;
    QVariantMap snapshot() const { return state; }
    QString error() const;
    bool busy() const { return action.state() != QProcess::NotRunning || !pending.isEmpty(); }
    Q_INVOKABLE void setMute(int channel, bool muted);
    Q_INVOKABLE void setLighting(const QString &key, const QString &value);
    Q_INVOKABLE void route(const QString &application, const QString &channel);
    Q_INVOKABLE void initialize();
    Q_INVOKABLE void startDriver();
    Q_INVOKABLE void refresh();
signals:
    void snapshotChanged();
    void errorChanged();
    void busyChanged();
private:
    void enqueue(QStringList args);
    void dispatch();
    QString executable, loadError, actionError;
    QVariantMap state;
    QProcess poll, action;
    QTimer pollTimer, pollTimeout, actionTimeout;
    QList<QStringList> pending;
};
