// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Musik Authors

#include "mpriscontroller.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QTimer>

static const QString MPRIS_SERVICE = QStringLiteral("org.mpris.MediaPlayer2.org.koderoots.musik");
static const QString MPRIS_PATH = QStringLiteral("/org/mpris/MediaPlayer2");

MprisController::MprisController(QObject *parent)
    : QObject(parent)
{
}

MprisController::~MprisController() = default;

QString MprisController::title() const { return m_title; }
void MprisController::setTitle(const QString &title)
{
    if (m_title != title) {
        m_title = title;
        Q_EMIT metadataChanged();
    }
}

QString MprisController::artist() const { return m_artist; }
void MprisController::setArtist(const QString &artist)
{
    if (m_artist != artist) {
        m_artist = artist;
        Q_EMIT metadataChanged();
    }
}

QString MprisController::album() const { return m_album; }
void MprisController::setAlbum(const QString &album)
{
    if (m_album != album) {
        m_album = album;
        Q_EMIT metadataChanged();
    }
}

QString MprisController::albumArtUrl() const { return m_albumArtUrl; }
void MprisController::setAlbumArtUrl(const QString &url)
{
    if (m_albumArtUrl != url) {
        m_albumArtUrl = url;
        Q_EMIT metadataChanged();
    }
}

qint64 MprisController::duration() const { return m_duration; }
void MprisController::setDuration(qint64 duration)
{
    if (m_duration != duration) {
        m_duration = duration;
        Q_EMIT metadataChanged();
    }
}

qint64 MprisController::position() const { return m_position; }
void MprisController::setPosition(qint64 position)
{
    if (m_position != position) {
        m_position = position;
        Q_EMIT positionChanged();
    }
}

int MprisController::playbackState() const { return m_playbackState; }
void MprisController::setPlaybackState(int state)
{
    if (m_playbackState != state) {
        m_playbackState = state;
        Q_EMIT playbackStateChanged();
    }
}

qreal MprisController::volume() const { return m_volume; }
void MprisController::setVolume(qreal volume)
{
    if (!qFuzzyCompare(m_volume, volume)) {
        m_volume = volume;
        Q_EMIT volumeChanged();
    }
}

bool MprisController::canPlay() const { return m_canPlay; }
void MprisController::setCanPlay(bool can)
{
    if (m_canPlay != can) {
        m_canPlay = can;
        Q_EMIT canPlayChanged();
    }
}

bool MprisController::canPause() const { return m_canPause; }
void MprisController::setCanPause(bool can)
{
    if (m_canPause != can) {
        m_canPause = can;
        Q_EMIT canPauseChanged();
    }
}

bool MprisController::canGoNext() const { return m_canGoNext; }
void MprisController::setCanGoNext(bool can)
{
    if (m_canGoNext != can) {
        m_canGoNext = can;
        Q_EMIT canGoNextChanged();
    }
}

bool MprisController::canGoPrevious() const { return m_canGoPrevious; }
void MprisController::setCanGoPrevious(bool can)
{
    if (m_canGoPrevious != can) {
        m_canGoPrevious = can;
        Q_EMIT canGoPreviousChanged();
    }
}

bool MprisController::canSeek() const { return m_canSeek; }
void MprisController::setCanSeek(bool can)
{
    if (m_canSeek != can) {
        m_canSeek = can;
        Q_EMIT canSeekChanged();
    }
}

int MprisController::repeatMode() const { return m_repeatMode; }
void MprisController::setRepeatMode(int mode)
{
    if (m_repeatMode != mode) {
        m_repeatMode = mode;
        Q_EMIT repeatModeChanged();
    }
}

bool MprisController::shuffle() const { return m_shuffle; }
void MprisController::setShuffle(bool shuffle)
{
    if (m_shuffle != shuffle) {
        m_shuffle = shuffle;
        Q_EMIT shuffleChanged();
    }
}

void MprisController::registerService()
{
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.registerService(MPRIS_SERVICE)) {
        return;
    }

    new MprisRootAdaptor(this);
    new MprisPlayerAdaptor(this);

    bus.registerObject(MPRIS_PATH, this, QDBusConnection::ExportAdaptors);
}

void MprisController::updateMetadata()
{
    QVariantMap changed;
    changed[QStringLiteral("Metadata")] = currentMetadata();
    emitDbusPropertiesChanged(QStringLiteral("org.mpris.MediaPlayer2.Player"), changed);
}

void MprisController::updateCanControls()
{
    QVariantMap changed;
    changed[QStringLiteral("CanPlay")] = m_canPlay;
    changed[QStringLiteral("CanPause")] = m_canPause;
    changed[QStringLiteral("CanSeek")] = m_canSeek;
    changed[QStringLiteral("CanGoNext")] = m_canGoNext;
    changed[QStringLiteral("CanGoPrevious")] = m_canGoPrevious;
    emitDbusPropertiesChanged(QStringLiteral("org.mpris.MediaPlayer2.Player"), changed);
}

void MprisController::updatePlaybackState()
{
    QVariantMap changed;
    changed[QStringLiteral("PlaybackStatus")] = currentPlaybackStatus();
    emitDbusPropertiesChanged(QStringLiteral("org.mpris.MediaPlayer2.Player"), changed);
}

void MprisController::updateLoopAndShuffle()
{
    QVariantMap changed;
    changed[QStringLiteral("LoopStatus")] = currentLoopStatus();
    changed[QStringLiteral("Shuffle")] = m_shuffle;
    emitDbusPropertiesChanged(QStringLiteral("org.mpris.MediaPlayer2.Player"), changed);
}

void MprisController::updateVolume()
{
    QVariantMap changed;
    changed[QStringLiteral("Volume")] = QVariant(static_cast<double>(m_volume));
    emitDbusPropertiesChanged(QStringLiteral("org.mpris.MediaPlayer2.Player"), changed);
}

QVariantMap MprisController::currentMetadata() const
{
    QVariantMap metadata;
    metadata[QStringLiteral("mpris:trackid")] = QVariant::fromValue(QDBusObjectPath(QStringLiteral("/org/mpris/MediaPlayer2/Track/0")));
    metadata[QStringLiteral("mpris:length")] = m_duration * 1000;
    metadata[QStringLiteral("xesam:title")] = m_title;
    metadata[QStringLiteral("xesam:artist")] = QStringList{m_artist};
    metadata[QStringLiteral("xesam:album")] = m_album;
    metadata[QStringLiteral("xesam:url")] = QStringLiteral("musik://%1").arg(m_title);
    if (!m_albumArtUrl.isEmpty()) {
        QString artUrl = m_albumArtUrl;
        if (!artUrl.startsWith(QStringLiteral("file://"))) {
            artUrl = QStringLiteral("file://") + artUrl;
        }
        metadata[QStringLiteral("mpris:artUrl")] = artUrl;
    }
    return metadata;
}

QString MprisController::currentPlaybackStatus() const
{
    switch (m_playbackState) {
    case Playing:
        return QStringLiteral("Playing");
    case Paused:
        return QStringLiteral("Paused");
    default:
        return QStringLiteral("Stopped");
    }
}

QString MprisController::currentLoopStatus() const
{
    if (m_repeatMode == 1) return QStringLiteral("Track");
    if (m_repeatMode == 2) return QStringLiteral("Playlist");
    return QStringLiteral("None");
}

void MprisController::emitDbusPropertiesChanged(const QString &interface, const QVariantMap &changed)
{
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        return;
    }

    QDBusMessage signal = QDBusMessage::createSignal(
        MPRIS_PATH,
        QStringLiteral("org.freedesktop.DBus.Properties"),
        QStringLiteral("PropertiesChanged"));
    signal << interface << changed << QStringList();
    bus.send(signal);
}

// ---- MprisRootAdaptor ----

MprisRootAdaptor::MprisRootAdaptor(MprisController *parent)
    : QDBusAbstractAdaptor(parent)
{
}

MprisRootAdaptor::~MprisRootAdaptor() = default;

void MprisRootAdaptor::Raise()
{
}

void MprisRootAdaptor::Quit()
{
    QCoreApplication::quit();
}

// ---- MprisPlayerAdaptor ----

MprisPlayerAdaptor::MprisPlayerAdaptor(MprisController *parent)
    : QDBusAbstractAdaptor(parent)
    , m_controller(parent)
{
}

MprisPlayerAdaptor::~MprisPlayerAdaptor() = default;

QString MprisPlayerAdaptor::PlaybackStatus() const
{
    return m_controller->currentPlaybackStatus();
}

QVariantMap MprisPlayerAdaptor::Metadata() const
{
    return m_controller->currentMetadata();
}

double MprisPlayerAdaptor::Volume() const
{
    return m_controller->volume();
}

void MprisPlayerAdaptor::SetVolume(double volume)
{
    m_controller->setVolume(volume);
    Q_EMIT m_controller->setVolumeRequested(volume);
}

QString MprisPlayerAdaptor::LoopStatus() const
{
    return m_controller->currentLoopStatus();
}

void MprisPlayerAdaptor::SetLoopStatus(const QString &status)
{
    if (status == QLatin1String("Track"))
        m_controller->setRepeatMode(1);
    else if (status == QLatin1String("Playlist"))
        m_controller->setRepeatMode(2);
    else
        m_controller->setRepeatMode(0);
}

bool MprisPlayerAdaptor::Shuffle() const
{
    return m_controller->shuffle();
}

void MprisPlayerAdaptor::SetShuffle(bool shuffle)
{
    m_controller->setShuffle(shuffle);
}

bool MprisPlayerAdaptor::CanGoNext() const
{
    return m_controller->canGoNext();
}

bool MprisPlayerAdaptor::CanGoPrevious() const
{
    return m_controller->canGoPrevious();
}

bool MprisPlayerAdaptor::CanPlay() const
{
    return m_controller->canPlay();
}

bool MprisPlayerAdaptor::CanPause() const
{
    return m_controller->canPause();
}

bool MprisPlayerAdaptor::CanSeek() const
{
    return m_controller->canSeek();
}

qint64 MprisPlayerAdaptor::Position() const
{
    return m_controller->position() * 1000;
}

void MprisPlayerAdaptor::Play()
{
    Q_EMIT m_controller->playRequested();
}

void MprisPlayerAdaptor::Pause()
{
    Q_EMIT m_controller->pauseRequested();
}

void MprisPlayerAdaptor::PlayPause()
{
    Q_EMIT m_controller->playPauseRequested();
}

void MprisPlayerAdaptor::Stop()
{
    Q_EMIT m_controller->stopRequested();
}

void MprisPlayerAdaptor::Next()
{
    Q_EMIT m_controller->nextRequested();
}

void MprisPlayerAdaptor::Previous()
{
    Q_EMIT m_controller->previousRequested();
}

void MprisPlayerAdaptor::Seek(qint64 offset)
{
    Q_EMIT m_controller->seekRequested(offset);
}

void MprisPlayerAdaptor::SetPosition(const QDBusObjectPath &trackId, qint64 position)
{
    Q_UNUSED(trackId);
    Q_EMIT m_controller->setPositionRequested(position / 1000);
}

void MprisPlayerAdaptor::OpenUri(const QString &uri)
{
    Q_UNUSED(uri);
}
