// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Musik Authors

#include "settings.h"

#include <KSharedConfig>

Settings::Settings(QObject *parent)
    : QObject(parent)
    , m_config(KSharedConfig::openConfig())
{
    loadSettings();
}

int Settings::volume() const
{
    return m_volume;
}

void Settings::setVolume(int volume)
{
    if (volume < 0)
        volume = 0;
    if (volume > 100)
        volume = 100;

    if (m_volume != volume) {
        m_volume = volume;
        Q_EMIT volumeChanged();
        saveSettings();
    }
}

bool Settings::muted() const
{
    return m_muted;
}

void Settings::setMuted(bool muted)
{
    if (m_muted != muted) {
        m_muted = muted;
        Q_EMIT mutedChanged();
        saveSettings();
    }
}

bool Settings::showVolumeControls() const
{
    return m_showVolumeControls;
}

void Settings::setShowVolumeControls(bool show)
{
    if (m_showVolumeControls != show) {
        m_showVolumeControls = show;
        Q_EMIT showVolumeControlsChanged();
        saveSettings();
    }
}

void Settings::loadSettings()
{
    KConfigGroup group = m_config->group(QStringLiteral("Audio"));
    m_volume = group.readEntry(QStringLiteral("Volume"), 70);
    m_muted = group.readEntry(QStringLiteral("Muted"), false);
    m_showVolumeControls = group.readEntry(QStringLiteral("ShowVolumeControls"), true);
}

void Settings::saveSettings()
{
    KConfigGroup group = m_config->group(QStringLiteral("Audio"));
    group.writeEntry(QStringLiteral("Volume"), m_volume);
    group.writeEntry(QStringLiteral("Muted"), m_muted);
    group.writeEntry(QStringLiteral("ShowVolumeControls"), m_showVolumeControls);
    group.sync();
}
