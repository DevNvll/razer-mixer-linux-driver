#include "Controller.h"
#include <QCommandLineParser>
#include <QFileInfo>
#include <QGuiApplication>
#include <QFont>
#include <QPalette>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QStandardPaths>

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    app.setApplicationName("razer-mixer"); app.setApplicationDisplayName("Razer Audio Mixer");
    app.setOrganizationName("DevNvll"); app.setDesktopFileName("razer-mixer");
    QFont font("monospace"); font.setPixelSize(12); app.setFont(font);
    QPalette palette;
    palette.setColor(QPalette::Window, QColor("#000000"));
    palette.setColor(QPalette::WindowText, Qt::white);
    palette.setColor(QPalette::Base, QColor("#090909"));
    palette.setColor(QPalette::AlternateBase, QColor("#1a1a1a"));
    palette.setColor(QPalette::Text, Qt::white);
    palette.setColor(QPalette::Button, QColor("#1a1a1a"));
    palette.setColor(QPalette::ButtonText, Qt::white);
    palette.setColor(QPalette::Highlight, QColor("#505050"));
    palette.setColor(QPalette::HighlightedText, Qt::white);
    app.setPalette(palette);
    QQuickStyle::setStyle("Basic");
    QCommandLineParser parser; parser.addHelpOption();
    parser.addOption({"driver-cli", "Path to the native driver executable", "path"}); parser.process(app);
    auto driver = parser.value("driver-cli");
    if (driver.isEmpty()) driver = QCoreApplication::applicationDirPath() + "/razer-mixer-driver";
    if (!QFileInfo::exists(driver)) driver = QStandardPaths::findExecutable("razer-mixer-driver");
    Controller controller(driver);
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("Mixer", &controller);
    engine.load(QUrl("qrc:/qt/qml/RazerMixer/app/qml/Main.qml"));
    if (engine.rootObjects().isEmpty()) return 1;
    return app.exec();
}
