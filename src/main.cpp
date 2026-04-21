#include <QApplication>
#include <QIcon>
#include <KAboutData>
#include <KIconTheme>
#include <KLocalizedString>
#include <QQmlApplicationEngine>
#include <QUrl>
#include <QQuickStyle>
#include <QQmlContext>
#include <QQmlEngine>
#include <QString>
#include <QCommandLineParser>
#include <QFileInfo>
#include <QDir>
#include <QMetaObject>

#include <KLocalizedContext>
#include <QCoreApplication>

#ifndef MUSIK_VERSION_STRING
#define MUSIK_VERSION_STRING "0.1.1"
#endif

int main(int argc, char *argv[])
{
    KIconTheme::initTheme();
    qputenv("QT_MEDIA_BACKEND", "gstreamer");
    qputenv("QT_AUDIO_BACKEND", "pulseaudio");
    QApplication app(argc, argv);
    KLocalizedString::setApplicationDomain("musik");

    KAboutData aboutData(
        QStringLiteral("musik"),
        i18nc("@title", "Musik"),
        QStringLiteral(MUSIK_VERSION_STRING),
        i18n("A simple audio player"),
        KAboutLicense::GPL_V3,
        i18n("© 2026 KodeRoots"));
    aboutData.setBugAddress("https://github.com/KodeRoots/Musik/issues");
    aboutData.addAuthor(
        i18nc("@info:credit", "Denys Madureira"),
        i18nc("@info:credit", "Author"),
        QStringLiteral("denys@koderoots.org"),
        QStringLiteral("https://denysmadureira.dev"));
    aboutData.setTranslator(
        i18nc("NAME OF TRANSLATORS", "Your names"),
        i18nc("EMAIL OF TRANSLATORS", "Your emails"));
    KAboutData::setApplicationData(aboutData);
    QApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral("org.koderoots.musik")));

    QApplication::setApplicationName(aboutData.componentName());
    QApplication::setApplicationVersion(aboutData.version());
    QApplication::setDesktopFileName(QStringLiteral("org.koderoots.musik"));
    QApplication::setStyle(QStringLiteral("breeze"));
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
        QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));
    }

    QCommandLineParser parser;
    parser.setApplicationDescription(aboutData.shortDescription());
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(QStringLiteral("files"), i18n("Audio files to open."), QStringLiteral("[files...]"));
    parser.process(app);

    QStringList fileUrls;
    for (const QString &arg : parser.positionalArguments()) {
        QFileInfo fileInfo(arg);
        if (fileInfo.exists()) {
            fileUrls.append(QUrl::fromLocalFile(fileInfo.absoluteFilePath()).toString());
        }
    }

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));

    engine.loadFromModule("org.koderoots.musik", "Main");

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    if (!fileUrls.isEmpty()) {
        QObject *rootObject = engine.rootObjects().first();
        QMetaObject::invokeMethod(rootObject, "openFiles",
                                  Q_ARG(QVariant, QVariant::fromValue(fileUrls)));
    }

    return app.exec();
}
