// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Musik Authors

#ifndef MPRISCONTROLLER_H
#define MPRISCONTROLLER_H

#include <QDBusAbstractAdaptor>
#include <QDBusObjectPath>
#include <QObject>
#include <QQmlEngine>

class MprisController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY metadataChanged)
    Q_PROPERTY(QString artist READ artist WRITE setArtist NOTIFY metadataChanged)
    Q_PROPERTY(QString album READ album WRITE setAlbum NOTIFY metadataChanged)
    Q_PROPERTY(QString albumArtUrl READ albumArtUrl WRITE setAlbumArtUrl NOTIFY metadataChanged)
    Q_PROPERTY(qint64 duration READ duration WRITE setDuration NOTIFY metadataChanged)
    Q_PROPERTY(qint64 position READ position WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(int playbackState READ playbackState WRITE setPlaybackState NOTIFY playbackStateChanged)
    Q_PROPERTY(qreal volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool canPlay READ canPlay WRITE setCanPlay NOTIFY canPlayChanged)
    Q_PROPERTY(bool canPause READ canPause WRITE setCanPause NOTIFY canPauseChanged)
    Q_PROPERTY(bool canGoNext READ canGoNext WRITE setCanGoNext NOTIFY canGoNextChanged)
    Q_PROPERTY(bool canGoPrevious READ canGoPrevious WRITE setCanGoPrevious NOTIFY canGoPreviousChanged)
    Q_PROPERTY(bool canSeek READ canSeek WRITE setCanSeek NOTIFY canSeekChanged)
    Q_PROPERTY(int repeatMode READ repeatMode WRITE setRepeatMode NOTIFY repeatModeChanged)
    Q_PROPERTY(bool shuffle READ shuffle WRITE setShuffle NOTIFY shuffleChanged)

public:
    enum PlaybackState { Stopped = 0, Playing = 1, Paused = 2 };
    Q_ENUM(PlaybackState)

    explicit MprisController(QObject *parent = nullptr);
    ~MprisController() override;

    static MprisController *create(QQmlEngine *, QJSEngine *)
    {
        return new MprisController;
    }

    QString title() const;
    void setTitle(const QString &title);

    QString artist() const;
    void setArtist(const QString &artist);

    QString album() const;
    void setAlbum(const QString &album);

    QString albumArtUrl() const;
    void setAlbumArtUrl(const QString &url);

    qint64 duration() const;
    void setDuration(qint64 duration);

    qint64 position() const;
    void setPosition(qint64 position);

    int playbackState() const;
    void setPlaybackState(int state);

    qreal volume() const;
    void setVolume(qreal volume);

    bool canPlay() const;
    void setCanPlay(bool can);

    bool canPause() const;
    void setCanPause(bool can);

    bool canGoNext() const;
    void setCanGoNext(bool can);

    bool canGoPrevious() const;
    void setCanGoPrevious(bool can);

    bool canSeek() const;
    void setCanSeek(bool can);

    int repeatMode() const;
    void setRepeatMode(int mode);

    bool shuffle() const;
    void setShuffle(bool shuffle);

    Q_INVOKABLE void registerService();
    Q_INVOKABLE void updateMetadata();
    Q_INVOKABLE void updateCanControls();
    Q_INVOKABLE void updatePlaybackState();
    Q_INVOKABLE void updateLoopAndShuffle();
    Q_INVOKABLE void updateVolume();

    QVariantMap currentMetadata() const;
    QString currentPlaybackStatus() const;
    QString currentLoopStatus() const;

Q_SIGNALS:
    void metadataChanged();
    void positionChanged();
    void playbackStateChanged();
    void volumeChanged();
    void canPlayChanged();
    void canPauseChanged();
    void canGoNextChanged();
    void canGoPreviousChanged();
    void canSeekChanged();
    void repeatModeChanged();
    void shuffleChanged();
    void playRequested();
    void pauseRequested();
    void playPauseRequested();
    void stopRequested();
    void nextRequested();
    void previousRequested();
    void seekRequested(qint64 offset);
    void setVolumeRequested(qreal volume);
    void setPositionRequested(qint64 position);

private:
    void emitDbusPropertiesChanged(const QString &interface, const QVariantMap &changed);

    QString m_title;
    QString m_artist;
    QString m_album;
    QString m_albumArtUrl;
    qint64 m_duration = 0;
    qint64 m_position = 0;
    int m_playbackState = Stopped;
    qreal m_volume = 1.0;
    bool m_canPlay = false;
    bool m_canPause = false;
    bool m_canGoNext = false;
    bool m_canGoPrevious = false;
    bool m_canSeek = false;
    int m_repeatMode = 0;
    bool m_shuffle = false;
};

class MprisRootAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2")
    Q_PROPERTY(bool CanRaise READ CanRaise CONSTANT)
    Q_PROPERTY(bool CanQuit READ CanQuit CONSTANT)
    Q_PROPERTY(bool CanFullscreen READ CanFullscreen CONSTANT)
    Q_PROPERTY(bool CanSetFullscreen READ CanSetFullscreen CONSTANT)
    Q_PROPERTY(bool HasTrackList READ HasTrackList CONSTANT)
    Q_PROPERTY(QString Identity READ Identity CONSTANT)
    Q_PROPERTY(QString DesktopEntry READ DesktopEntry CONSTANT)
    Q_PROPERTY(QStringList SupportedUriSchemes READ SupportedUriSchemes CONSTANT)
    Q_PROPERTY(QStringList SupportedMimeTypes READ SupportedMimeTypes CONSTANT)

public:
    explicit MprisRootAdaptor(MprisController *parent);
    ~MprisRootAdaptor() override;

    bool CanRaise() const { return true; }
    bool CanQuit() const { return true; }
    bool CanFullscreen() const { return false; }
    bool CanSetFullscreen() const { return false; }
    bool HasTrackList() const { return false; }
    QString Identity() const { return QStringLiteral("Musik"); }
    QString DesktopEntry() const { return QStringLiteral("org.koderoots.musik"); }
    QStringList SupportedUriSchemes() const { return {QStringLiteral("file")}; }
    QStringList SupportedMimeTypes() const
    {
        return {QStringLiteral("audio/mpeg"), QStringLiteral("audio/flac"), QStringLiteral("audio/ogg"),
                QStringLiteral("audio/x-wav"), QStringLiteral("audio/mp4"), QStringLiteral("audio/aac"),
                QStringLiteral("audio/opus")};
    }

public Q_SLOTS:
    void Raise();
    void Quit();
};

class MprisPlayerAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2.Player")
    Q_PROPERTY(QString PlaybackStatus READ PlaybackStatus NOTIFY PropertiesChanged)
    Q_PROPERTY(QVariantMap Metadata READ Metadata NOTIFY PropertiesChanged)
    Q_PROPERTY(double Volume READ Volume WRITE SetVolume NOTIFY PropertiesChanged)
    Q_PROPERTY(QString LoopStatus READ LoopStatus WRITE SetLoopStatus NOTIFY PropertiesChanged)
    Q_PROPERTY(bool Shuffle READ Shuffle WRITE SetShuffle NOTIFY PropertiesChanged)
    Q_PROPERTY(bool CanGoNext READ CanGoNext NOTIFY PropertiesChanged)
    Q_PROPERTY(bool CanGoPrevious READ CanGoPrevious NOTIFY PropertiesChanged)
    Q_PROPERTY(bool CanPlay READ CanPlay NOTIFY PropertiesChanged)
    Q_PROPERTY(bool CanPause READ CanPause NOTIFY PropertiesChanged)
    Q_PROPERTY(bool CanSeek READ CanSeek NOTIFY PropertiesChanged)
    Q_PROPERTY(bool CanControl READ CanControl CONSTANT)
    Q_PROPERTY(qint64 Position READ Position NOTIFY PropertiesChanged)

public:
    explicit MprisPlayerAdaptor(MprisController *parent);
    ~MprisPlayerAdaptor() override;

    QString PlaybackStatus() const;
    QVariantMap Metadata() const;
    double Volume() const;
    void SetVolume(double volume);
    QString LoopStatus() const;
    void SetLoopStatus(const QString &status);
    bool Shuffle() const;
    void SetShuffle(bool shuffle);
    bool CanGoNext() const;
    bool CanGoPrevious() const;
    bool CanPlay() const;
    bool CanPause() const;
    bool CanSeek() const;
    bool CanControl() const { return true; }
    qint64 Position() const;

Q_SIGNALS:
    void PropertiesChanged(const QString &interface, const QVariantMap &changed, const QStringList &invalidated);

    void Seeked(qint64 position);

public Q_SLOTS:
    void Play();
    void Pause();
    void PlayPause();
    void Stop();
    void Next();
    void Previous();
    void Seek(qint64 offset);
    void SetPosition(const QDBusObjectPath &trackId, qint64 position);
    void OpenUri(const QString &uri);

private:
    MprisController *m_controller;
};

#endif // MPRISCONTROLLER_H