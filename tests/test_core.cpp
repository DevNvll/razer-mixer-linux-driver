#include "Audio.h"
#include "Config.h"
#include "Protocol.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QtTest>

class CoreTests : public QObject {
    Q_OBJECT
private slots:
    void capturedInitialization() {
        const auto fixture = Mixer::readJson(QFINDTESTDATA("fixtures/fader-initialization.json"));
        QCOMPARE(Mixer::faderQuery(), QByteArray::fromHex(fixture.value("query").toString().toLatin1()));
        QCOMPARE(Mixer::faderEnable(), QByteArray::fromHex(fixture.value("enable").toString().toLatin1()));
        QCOMPARE(Mixer::faderEnable().size(), 64);
        QCOMPARE(Mixer::faderEnable().at(62), char(0));
    }
    void capturedLightingChecksum() {
        const auto report = Mixer::feature(15, 4, QByteArray::fromHex("010464"));
        QCOMPARE(report.left(12), QByteArray::fromHex("07001f000000030f04010464"));
        QCOMPARE(report.at(62), char(0x69));
        QVERIFY_EXCEPTION_THROWN(Mixer::feature(15, 2, QByteArray(54, 0)), std::runtime_error);
    }
    void coldBootButtonsPreserveVolume() {
        Mixer::ControlChanges changes;
        const auto press = *Mixer::parseControls(QByteArray::fromHex("0901000000000004"));
        auto result = changes.update(press);
        QCOMPARE(result.pressed, 1); QVERIFY(result.volumes.isEmpty());
        QCOMPARE(changes.update(press).pressed, 0); QVERIFY(!changes.faders);
    }
    void simultaneousButtonEdges() {
        Mixer::ControlChanges changes;
        const int masks[]{3,3,2,6,0,1}, expected[]{3,0,0,4,0,1};
        for (int i = 0; i < 6; ++i) {
            auto data = QByteArray::fromHex("0900000000000004"); data[1] = masks[i];
            QCOMPARE(changes.update(*Mixer::parseControls(data)).pressed, expected[i]);
        }
    }
    void initialPositionDoesNotChangeVolume() {
        Mixer::ControlChanges changes;
        QVERIFY(changes.update(*Mixer::parseControls(QByteArray::fromHex("0900003232323204"))).volumes.isEmpty());
        const auto result = changes.update(*Mixer::parseControls(QByteArray::fromHex("0900003200643204")));
        QCOMPARE(result.volumes.size(), 2); QCOMPARE(result.volumes.value(1), 0.0); QCOMPARE(result.volumes.value(2), 1.0);
    }
    void enabledZeroAndOnePercent() {
        Mixer::ControlChanges changes; changes.reportingEnabled = true;
        QVERIFY(changes.update(*Mixer::parseControls(QByteArray::fromHex("0900000000000004"))).volumes.isEmpty());
        QVERIFY(changes.faders.has_value());
        QCOMPARE(changes.update(*Mixer::parseControls(QByteArray::fromHex("0900000001000004"))).volumes.value(1), 0.01);
        QCOMPARE(changes.update(*Mixer::parseControls(QByteArray::fromHex("0900000000000004"))).volumes.value(1), 0.0);
    }
    void everyDownwardEndpoint() {
        Mixer::ControlChanges changes; changes.reportingEnabled = true;
        changes.update(*Mixer::parseControls(QByteArray::fromHex("0900006464646404")));
        for (int value = 99; value >= 0; --value) {
            auto data = QByteArray::fromHex("0900000000000004"); for (int i = 3; i < 7; ++i) data[i] = value;
            const auto result = changes.update(*Mixer::parseControls(data));
            QCOMPARE(result.volumes.size(), 4);
            for (int i = 0; i < 4; ++i) QCOMPARE(result.volumes.value(i), value / 100.0);
        }
    }
    void invalidReports() {
        QVERIFY(!Mixer::parseControls(QByteArray::fromHex("09")));
        QVERIFY(!Mixer::parseControls(QByteArray::fromHex("0100003232323204")));
        QVERIFY(!Mixer::parseControls(QByteArray::fromHex("090000ff32323204"))->faders);
    }
    void explicitRouteOverridesDefault() {
        auto config = Mixer::defaults(); config["routes"] = QJsonObject{{"spotify", "master"}};
        QCOMPARE(Mixer::applicationChannel({{"application.process.binary", "Spotify"}}, config), config.value("output").toString());
    }
    void processIdentityTakesPriority() {
        auto config = Mixer::defaults(); config["routes"] = QJsonObject{{"discord", "music"}, {"webrtc voiceengine", "master"}};
        QCOMPARE(Mixer::applicationChannel({{"application.process.binary", "Discord"}, {"application.name", "WEBRTC VoiceEngine"}}, config), "razer_mixer_music");
    }
    void applicationDefaults() {
        QCOMPARE(Mixer::applicationChannel({{"application.name", "Spotify"}}, Mixer::defaults()), "razer_mixer_music");
        QCOMPARE(Mixer::applicationChannel({{"application.process.binary", "vesktop"}}, Mixer::defaults()), "razer_mixer_chat");
        QVERIFY(Mixer::applicationChannel({{"application.name", "unassigned"}}, Mixer::defaults()).isEmpty());
    }
    void moduleArgumentsMatchWholeValues() {
        QCOMPARE(Mixer::moduleArgument("source=razer_mixer_chat.monitor sink=output-extra", "sink"), "output-extra");
        QCOMPARE(Mixer::moduleArgument("sink=\"output one\" source='input two'", "source"), "input two");
        QVERIFY(Mixer::moduleArgument("other_sink=output", "sink").isEmpty());
    }
    void invalidSettings_data() {
        QTest::addColumn<QString>("key"); QTest::addColumn<QJsonValue>("value");
        QTest::newRow("high brightness") << QString("brightness") << QJsonValue(101);
        QTest::newRow("negative brightness") << QString("brightness") << QJsonValue(-1);
        QTest::newRow("fractional brightness") << QString("brightness") << QJsonValue(50.5);
        QTest::newRow("boolean brightness") << QString("brightness") << QJsonValue(true);
        QTest::newRow("bad color") << QString("color") << QJsonValue("red");
        QTest::newRow("numeric color") << QString("color") << QJsonValue(123);
        QTest::newRow("missing output") << QString("output") << QJsonValue("");
        QTest::newRow("bad route") << QString("routes") << QJsonValue(QJsonObject{{"spotify", "unknown"}});
    }
    void invalidSettings() {
        QFETCH(QString, key); QFETCH(QJsonValue, value);
        QTemporaryDir directory; const auto path = directory.path() + "/config.json";
        Mixer::writeJson(path, Mixer::defaults());
        QVERIFY_EXCEPTION_THROWN(Mixer::editConfig([&](QJsonObject &config) { config[key] = value; }, path), std::runtime_error);
        QCOMPARE(Mixer::loadConfig(path), Mixer::defaults());
    }
    void independentEditsPreserveSettings() {
        QTemporaryDir directory; const auto path = directory.path() + "/config.json";
        Mixer::writeJson(path, Mixer::defaults());
        Mixer::editConfig([](QJsonObject &config) { config["color"] = "#ABCDEF"; }, path);
        Mixer::editConfig([](QJsonObject &config) { config["routes"] = QJsonObject{{"spotify", "chat"}}; }, path);
        const auto saved = Mixer::loadConfig(path);
        QCOMPARE(saved.value("color").toString(), "abcdef");
        QCOMPARE(saved.value("brightness").toInt(), 50);
        QCOMPARE(saved.value("routes").toObject().value("spotify").toString(), "chat");
    }
};
QTEST_GUILESS_MAIN(CoreTests)
#include "test_core.moc"
