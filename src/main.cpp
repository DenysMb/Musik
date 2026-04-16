#include <QApplication>
#include <KIconTheme>
#include <KLocalizedString>
#include <QQmlApplicationEngine>
#include <QUrl>
#include <QQuickStyle>
#include <QQmlContext>
#include <QQmlEngine>

#include <KLocalizedContext>
#include <QCoreApplication>

int main(int argc, char *argv[])
{
    KIconTheme::initTheme();
    qputenv("QT_MEDIA_BACKEND", "gstreamer");
    // Force PulseAudio backend for device enumeration to support Bluetooth audio
    // Qt's GStreamer audio path doesn't enumerate Bluetooth devices properly (QTBUG-116015)
    // PulseAudio (via PipeWire's compatibility layer) lists all devices including Bluetooth
    qputenv("QT_AUDIO_BACKEND", "pulseaudio");
    QApplication app(argc, argv);
    KLocalizedString::setApplicationDomain("musik");
    QApplication::setApplicationName(QStringLiteral("Musik"));
    QApplication::setDesktopFileName(QStringLiteral("org.koderoots.musik.desktop"));
    QApplication::setStyle(QStringLiteral("breeze"));
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
        QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));
    }

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));

    engine.loadFromModule("org.koderoots.musik", "Main");

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
