// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Musik Authors

#ifndef SETTINGS_H
#define SETTINGS_H

#include <KConfigGroup>
#include <KSharedConfig>
#include <QObject>
#include <QQmlEngine>

class Settings : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool muted READ muted WRITE setMuted NOTIFY mutedChanged)

public:
    explicit Settings(QObject *parent = nullptr);

    int volume() const;
    void setVolume(int volume);

    bool muted() const;
    void setMuted(bool muted);

    static Settings *create(QQmlEngine *, QJSEngine *)
    {
        return new Settings;
    }

Q_SIGNALS:
    void volumeChanged();
    void mutedChanged();

private:
    void loadSettings();
    void saveSettings();

    KSharedConfig::Ptr m_config;
    int m_volume = 70;
    bool m_muted = false;
};

#endif // SETTINGS_H
