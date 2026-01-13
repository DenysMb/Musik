// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Musik Authors

#include "playlistmodel.h"

#include <QFile>
#include <QFileInfo>
#include <QRandomGenerator>
#include <QStandardPaths>
#include <QTemporaryFile>

#include <KLocalizedString>

#include <fileref.h>
#include <tag.h>
#include <tpropertymap.h>

PlaylistModel::PlaylistModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

PlaylistModel::~PlaylistModel()
{
    cleanupTempFiles();
}

int PlaylistModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_tracks.size();
}

QVariant PlaylistModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_tracks.size()) {
        return QVariant();
    }

    const Track &track = m_tracks.at(index.row());

    switch (role) {
    case UrlRole:
        return track.url;
    case TitleRole:
        return track.title;
    case ArtistRole:
        return track.artist;
    case AlbumRole:
        return track.album;
    case AlbumArtRole:
        return track.albumArtPath;
    case DurationRole:
        return track.duration;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> PlaylistModel::roleNames() const
{
    return {
        {UrlRole, "url"},
        {TitleRole, "title"},
        {ArtistRole, "artist"},
        {AlbumRole, "album"},
        {AlbumArtRole, "albumArt"},
        {DurationRole, "duration"}
    };
}

int PlaylistModel::count() const
{
    return m_tracks.size();
}

int PlaylistModel::currentIndex() const
{
    return m_currentIndex;
}

void PlaylistModel::setCurrentIndex(int index)
{
    if (index < -1) {
        index = -1;
    }
    if (index >= m_tracks.size()) {
        index = m_tracks.size() - 1;
    }
    if (m_currentIndex != index) {
        m_currentIndex = index;
        Q_EMIT currentIndexChanged();
    }
}

QUrl PlaylistModel::currentUrl() const
{
    if (m_currentIndex >= 0 && m_currentIndex < m_tracks.size()) {
        return m_tracks.at(m_currentIndex).url;
    }
    return QUrl();
}

void PlaylistModel::addTracks(const QList<QUrl> &urls)
{
    if (urls.isEmpty()) {
        return;
    }

    QList<Track> newTracks;
    for (const QUrl &url : urls) {
        if (!url.isLocalFile()) {
            continue;
        }

        QString path = url.toLocalFile();
        if (!QFile::exists(path)) {
            Q_EMIT errorOccurred(i18n("File not found: %1", path));
            continue;
        }

        if (!isAudioFile(path)) {
            continue;
        }

        Track track = extractTrackMetadata(url);
        newTracks.append(track);
    }

    if (newTracks.isEmpty()) {
        return;
    }

    int first = m_tracks.size();
    int last = first + newTracks.size() - 1;

    beginInsertRows(QModelIndex(), first, last);
    m_tracks.append(newTracks);
    endInsertRows();

    Q_EMIT countChanged();

    // Auto-select first track if playlist was empty
    if (m_currentIndex < 0 && !m_tracks.isEmpty()) {
        setCurrentIndex(0);
    }

    // Emit trackAdded for each new track
    for (int i = first; i <= last; ++i) {
        Q_EMIT trackAdded(i);
    }
}

void PlaylistModel::removeTrack(int index)
{
    if (index < 0 || index >= m_tracks.size()) {
        return;
    }

    // Clean up temp album art file for this track
    const Track &track = m_tracks.at(index);
    if (!track.albumArtPath.isEmpty()) {
        QString tempPath = track.albumArtPath;
        if (tempPath.startsWith(QStringLiteral("file://"))) {
            tempPath = tempPath.mid(7);
        }
        if (QFile::exists(tempPath)) {
            QFile::remove(tempPath);
            m_tempArtPaths.removeAll(tempPath);
        }
    }

    beginRemoveRows(QModelIndex(), index, index);
    m_tracks.removeAt(index);
    endRemoveRows();

    // Adjust current index if needed
    if (m_tracks.isEmpty()) {
        setCurrentIndex(-1);
    } else if (index < m_currentIndex) {
        // Track before current was removed, decrement
        m_currentIndex--;
        Q_EMIT currentIndexChanged();
    } else if (index == m_currentIndex) {
        // Current track was removed
        if (m_currentIndex >= m_tracks.size()) {
            setCurrentIndex(m_tracks.size() - 1);
        } else {
            // Stay at same index (which is now the next track)
            Q_EMIT currentIndexChanged();
        }
    }

    Q_EMIT countChanged();
}

void PlaylistModel::clear()
{
    if (m_tracks.isEmpty()) {
        return;
    }

    beginResetModel();
    cleanupTempFiles();
    m_tracks.clear();
    m_currentIndex = -1;
    endResetModel();

    Q_EMIT countChanged();
    Q_EMIT currentIndexChanged();
}

QUrl PlaylistModel::urlAt(int index) const
{
    if (index < 0 || index >= m_tracks.size()) {
        return QUrl();
    }
    return m_tracks.at(index).url;
}

int PlaylistModel::nextIndex(bool shuffle, int repeatMode) const
{
    if (m_tracks.isEmpty()) {
        return -1;
    }

    // RepeatMode: 0 = Off, 1 = One, 2 = All
    if (repeatMode == 1) {
        // Repeat current track
        return m_currentIndex;
    }

    if (shuffle) {
        // Random index excluding current (if more than one track)
        if (m_tracks.size() == 1) {
            return 0;
        }
        int next;
        do {
            next = QRandomGenerator::global()->bounded(m_tracks.size());
        } while (next == m_currentIndex);
        return next;
    }

    // Sequential next
    int next = m_currentIndex + 1;
    if (next >= m_tracks.size()) {
        if (repeatMode == 2) {
            // Repeat All: wrap to beginning
            return 0;
        }
        // Repeat Off: end of playlist
        return -1;
    }
    return next;
}

int PlaylistModel::previousIndex() const
{
    if (m_tracks.isEmpty() || m_currentIndex <= 0) {
        return m_currentIndex;
    }
    return m_currentIndex - 1;
}

Track PlaylistModel::extractTrackMetadata(const QUrl &url)
{
    Track track;
    track.url = url;

    QString path = url.toLocalFile();
    QFileInfo fileInfo(path);

    TagLib::FileRef file(path.toUtf8().constData());
    if (file.isNull()) {
        // Fallback to filename
        track.title = fileInfo.completeBaseName();
        track.artist = i18n("Unknown Artist");
        track.album = i18n("Unknown Album");
        return track;
    }

    TagLib::Tag *tag = file.tag();
    if (tag) {
        // Extract title
        if (tag->title().isEmpty()) {
            track.title = fileInfo.completeBaseName();
        } else {
            track.title = QString::fromStdString(tag->title().to8Bit(true));
        }

        // Extract artist
        if (tag->artist().isEmpty()) {
            track.artist = i18n("Unknown Artist");
        } else {
            track.artist = QString::fromStdString(tag->artist().to8Bit(true));
        }

        // Extract album
        if (tag->album().isEmpty()) {
            track.album = i18n("Unknown Album");
        } else {
            track.album = QString::fromStdString(tag->album().to8Bit(true));
        }
    } else {
        track.title = fileInfo.completeBaseName();
        track.artist = i18n("Unknown Artist");
        track.album = i18n("Unknown Album");
    }

    // Extract duration
    TagLib::AudioProperties *properties = file.audioProperties();
    if (properties) {
        track.duration = properties->lengthInMilliseconds();
    }

    // Extract album art
    TagLib::StringList complexKeys = file.complexPropertyKeys();
    if (complexKeys.contains("PICTURE")) {
        auto pictures = file.complexProperties("PICTURE");
        if (!pictures.isEmpty()) {
            const auto &picture = pictures[0];
            auto dataIt = picture.find("data");
            if (dataIt != picture.end()) {
                const TagLib::Variant &pictureData = dataIt->second;
                if (pictureData.type() == TagLib::Variant::ByteVector) {
                    TagLib::ByteVector data = pictureData.value<TagLib::ByteVector>();
                    if (!data.isEmpty()) {
                        // Determine file extension
                        QString extension = QStringLiteral(".jpg");
                        auto mimeIt = picture.find("mimeType");
                        if (mimeIt != picture.end()) {
                            QString mimeType = QString::fromStdString(mimeIt->second.value<TagLib::String>().to8Bit(true));
                            if (mimeType.contains(QStringLiteral("png"), Qt::CaseInsensitive)) {
                                extension = QStringLiteral(".png");
                            } else if (mimeType.contains(QStringLiteral("gif"), Qt::CaseInsensitive)) {
                                extension = QStringLiteral(".gif");
                            }
                        }

                        // Create temp file
                        QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
                        QString tempTemplate = tempDir + QStringLiteral("/musik_playlist_art_XXXXXX") + extension;

                        QTemporaryFile tempFile(tempTemplate);
                        tempFile.setAutoRemove(false);

                        if (tempFile.open()) {
                            tempFile.write(data.data(), data.size());
                            QString tempPath = tempFile.fileName();
                            tempFile.close();

                            track.albumArtPath = QStringLiteral("file://") + tempPath;
                            m_tempArtPaths.append(tempPath);
                        }
                    }
                }
            }
        }
    }

    return track;
}

void PlaylistModel::cleanupTempFiles()
{
    for (const QString &path : m_tempArtPaths) {
        if (QFile::exists(path)) {
            QFile::remove(path);
        }
    }
    m_tempArtPaths.clear();
}

bool PlaylistModel::isAudioFile(const QString &path) const
{
    static const QStringList extensions = {
        QStringLiteral(".mp3"),
        QStringLiteral(".flac"),
        QStringLiteral(".ogg"),
        QStringLiteral(".wav"),
        QStringLiteral(".m4a"),
        QStringLiteral(".aac"),
        QStringLiteral(".wma"),
        QStringLiteral(".opus")
    };

    QString lowerPath = path.toLower();
    for (const QString &ext : extensions) {
        if (lowerPath.endsWith(ext)) {
            return true;
        }
    }
    return false;
}
