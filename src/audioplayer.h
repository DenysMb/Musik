// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Musik Authors

#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QUrl>

/**
 * @brief Reads audio file metadata using TagLib and exposes it to QML
 *
 * AudioPlayer handles metadata extraction (title, artist, album) and
 * album art extraction to a temporary file for display in QML.
 */
class AudioPlayer : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString title READ title NOTIFY metadataChanged)
    Q_PROPERTY(QString artist READ artist NOTIFY metadataChanged)
    Q_PROPERTY(QString album READ album NOTIFY metadataChanged)
    Q_PROPERTY(QString albumArtPath READ albumArtPath NOTIFY metadataChanged)
    Q_PROPERTY(bool hasMetadata READ hasMetadata NOTIFY metadataChanged)

public:
    explicit AudioPlayer(QObject *parent = nullptr);
    ~AudioPlayer() override;

    QString title() const;
    QString artist() const;
    QString album() const;
    QString albumArtPath() const;
    bool hasMetadata() const;

    /**
     * @brief Loads metadata from an audio file
     * @param fileUrl The URL of the audio file to load
     * @return true if metadata was successfully read, false otherwise
     */
    Q_INVOKABLE bool loadFile(const QUrl &fileUrl);

    /**
     * @brief Clears all metadata and resets to empty state
     */
    Q_INVOKABLE void clearMetadata();

Q_SIGNALS:
    void metadataChanged();
    void errorOccurred(const QString &message);

private:
    void extractAlbumArt(const QString &filePath);
    QString extractFilenameAsTitle(const QString &filePath);
    void cleanupTempFile();

    QString m_title;
    QString m_artist;
    QString m_album;
    QString m_albumArtPath;
    QString m_tempArtPath;
    bool m_hasMetadata = false;
};

#endif // AUDIOPLAYER_H
