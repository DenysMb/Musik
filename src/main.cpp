#include <QApplication>
#include <KIconTheme>
#include <KLocalizedString>
#include <QQmlApplicationEngine>
#include <QUrl>
#include <QQuickStyle>
#include <QQmlContext>
#include <QQmlEngine>

#include <KLocalizedContext>

int main(int argc, char *argv[])
{
    KIconTheme::initTheme();
    QApplication app(argc, argv);
    KLocalizedString::setApplicationDomain("musik");
    QApplication::setApplicationName(QStringLiteral("Musik"));
    QApplication::setDesktopFileName(QStringLiteral("io.github.denysmb.musik.desktop"));
    QApplication::setStyle(QStringLiteral("breeze"));
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
        QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));
    }

    QQmlApplicationEngine engine;
    QQmlContext ctx(&engine);
    ctx.setContextObject(new KLocalizedContext(&engine));

    engine.loadFromModule("io.github.denysmb.musik", "Main");

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
