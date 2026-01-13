// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Musik Authors

#include "audioplayer.h"

#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTemporaryFile>

#include <KLocalizedString>

#include <fileref.h>
#include <tag.h>
#include <tpropertymap.h>

AudioPlayer::AudioPlayer(QObject *parent)
    : QObject(parent)
{
}

AudioPlayer::~AudioPlayer()
{
    cleanupTempFile();
}

QString AudioPlayer::title() const
{
    return m_title;
}

QString AudioPlayer::artist() const
{
    return m_artist;
}

QString AudioPlayer::album() const
{
    return m_album;
}

QString AudioPlayer::albumArtPath() const
{
    return m_albumArtPath;
}

bool AudioPlayer::hasMetadata() const
{
    return m_hasMetadata;
}

bool AudioPlayer::loadFile(const QUrl &fileUrl)
{
    // Clean up previous album art temp file
    cleanupTempFile();

    // Reset state
    m_title.clear();
    m_artist.clear();
    m_album.clear();
    m_albumArtPath.clear();
    m_hasMetadata = false;

    QString path = fileUrl.toLocalFile();
    if (path.isEmpty()) {
        Q_EMIT errorOccurred(i18n("Invalid file path"));
        Q_EMIT metadataChanged();
        return false;
    }

    // Check if file exists
    if (!QFile::exists(path)) {
        Q_EMIT errorOccurred(i18n("File not found: %1", path));
        Q_EMIT metadataChanged();
        return false;
    }

    TagLib::FileRef file(path.toUtf8().constData());
    if (file.isNull()) {
        Q_EMIT errorOccurred(i18n("Could not read file: %1", path));
        Q_EMIT metadataChanged();
        return false;
    }

    TagLib::Tag *tag = file.tag();
    if (tag) {
        // Extract title
        if (tag->title().isEmpty()) {
            m_title = extractFilenameAsTitle(path);
        } else {
            m_title = QString::fromStdString(tag->title().to8Bit(true));
        }

        // Extract artist
        if (tag->artist().isEmpty()) {
            m_artist = i18n("Unknown Artist");
        } else {
            m_artist = QString::fromStdString(tag->artist().to8Bit(true));
        }

        // Extract album
        if (tag->album().isEmpty()) {
            m_album = i18n("Unknown Album");
        } else {
            m_album = QString::fromStdString(tag->album().to8Bit(true));
        }

        m_hasMetadata = true;
    } else {
        // No tag at all, use fallbacks
        m_title = extractFilenameAsTitle(path);
        m_artist = i18n("Unknown Artist");
        m_album = i18n("Unknown Album");
        m_hasMetadata = true;
    }

    // Extract album art
    extractAlbumArt(path);

    Q_EMIT metadataChanged();
    return true;
}

void AudioPlayer::clearMetadata()
{
    cleanupTempFile();

    m_title.clear();
    m_artist.clear();
    m_album.clear();
    m_albumArtPath.clear();
    m_hasMetadata = false;

    Q_EMIT metadataChanged();
}

void AudioPlayer::extractAlbumArt(const QString &filePath)
{
    TagLib::FileRef file(filePath.toUtf8().constData());
    if (file.isNull()) {
        return;
    }

    TagLib::StringList complexKeys = file.complexPropertyKeys();
    if (!complexKeys.contains("PICTURE")) {
        return;
    }

    auto pictures = file.complexProperties("PICTURE");
    if (pictures.isEmpty()) {
        return;
    }

    // Get the first picture
    const auto &picture = pictures[0];
    auto dataIt = picture.find("data");
    if (dataIt == picture.end()) {
        return;
    }

    const TagLib::Variant &pictureData = dataIt->second;
    if (pictureData.type() != TagLib::Variant::ByteVector) {
        return;
    }

    TagLib::ByteVector data = pictureData.value<TagLib::ByteVector>();
    if (data.isEmpty()) {
        return;
    }

    // Determine file extension based on mime type if available
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
    m_tempArtPath = tempDir + QStringLiteral("/musik_albumart_XXXXXX") + extension;

    QTemporaryFile tempFile(m_tempArtPath);
    tempFile.setAutoRemove(false);

    if (!tempFile.open()) {
        return;
    }

    tempFile.write(data.data(), data.size());
    m_tempArtPath = tempFile.fileName();
    tempFile.close();

    // Set the path with file:// prefix for QML Image
    m_albumArtPath = QStringLiteral("file://") + m_tempArtPath;
}

QString AudioPlayer::extractFilenameAsTitle(const QString &filePath)
{
    QFileInfo fileInfo(filePath);
    return fileInfo.completeBaseName();
}

void AudioPlayer::cleanupTempFile()
{
    if (!m_tempArtPath.isEmpty() && QFile::exists(m_tempArtPath)) {
        QFile::remove(m_tempArtPath);
        m_tempArtPath.clear();
    }
}
