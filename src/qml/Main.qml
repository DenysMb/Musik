// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Musik Authors

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import QtQuick.Dialogs
import QtMultimedia
import org.kde.kirigami as Kirigami
import io.github.denysmb.musik

Kirigami.ApplicationWindow {
    id: root

    title: i18nc("@title:window", "Musik")
    width: 400
    height: 500
    minimumWidth: 400
    maximumWidth: 400
    minimumHeight: 500
    maximumHeight: 500

    // Time formatting helper function
    function formatTime(ms) {
        if (ms <= 0)
            return "0:00";
        var totalSeconds = Math.floor(ms / 1000);
        var minutes = Math.floor(totalSeconds / 60);
        var seconds = totalSeconds % 60;
        return minutes + ":" + (seconds < 10 ? "0" : "") + seconds;
    }

    // Show error notification
    function showError(message) {
        errorMessage.text = message;
        errorMessage.visible = true;
        errorHideTimer.restart();
    }

    // Timer for auto-hiding error messages
    Timer {
        id: errorHideTimer
        interval: 5000
        onTriggered: errorMessage.visible = false
    }

    // AudioPlayer C++ backend for metadata extraction
    AudioPlayer {
        id: audioPlayer

        onErrorOccurred: function (message) {
            showError(message);
        }
    }

    // MediaPlayer for audio playback
    MediaPlayer {
        id: mediaPlayer
        audioOutput: AudioOutput {}

        onPlaybackStateChanged: {
            // Stop and reset when track ends
            if (mediaPlayer.playbackState === MediaPlayer.StoppedState && mediaPlayer.position >= mediaPlayer.duration - 100 && mediaPlayer.duration > 0) {
                mediaPlayer.position = 0;
            }
        }

        onErrorOccurred: function (error, errorString) {
            if (error !== MediaPlayer.NoError) {
                showError(i18n("Playback error: %1", errorString));
            }
        }
    }

    // File dialog for opening audio files
    FileDialog {
        id: fileDialog
        title: i18n("Open Audio File")
        fileMode: FileDialog.OpenFile
        nameFilters: [i18n("Audio Files") + " (*.mp3 *.flac *.ogg *.wav *.m4a *.aac *.wma *.opus)", i18n("All Files") + " (*)"]
        onAccepted: {
            audioPlayer.loadFile(selectedFile);
            mediaPlayer.source = selectedFile;
            mediaPlayer.play();
        }
    }

    // Main content
    pageStack.initialPage: Kirigami.Page {
        id: mainPage

        title: i18nc("@title:window", "Musik")
        padding: Kirigami.Units.largeSpacing

        // Open action in the header toolbar
        actions: [
            Kirigami.Action {
                text: i18n("Open")
                icon.name: "document-open"
                onTriggered: fileDialog.open()
            }
        ]

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: Kirigami.Units.largeSpacing
            spacing: Kirigami.Units.largeSpacing

            // Album Art Area
            Item {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 280
                Layout.preferredHeight: 280

                Rectangle {
                    anchors.fill: parent
                    color: Kirigami.Theme.backgroundColor
                    border.color: Kirigami.Theme.disabledTextColor
                    border.width: 1
                    radius: Kirigami.Units.smallSpacing

                    // Album art image
                    Image {
                        id: albumArtImage
                        anchors.fill: parent
                        anchors.margins: 1
                        source: audioPlayer.albumArtPath
                        fillMode: Image.PreserveAspectFit
                        visible: audioPlayer.albumArtPath !== ""
                    }

                    // Placeholder icon when no album art
                    Kirigami.Icon {
                        anchors.centerIn: parent
                        width: 120
                        height: 120
                        source: "media-optical-audio"
                        opacity: 0.3
                        visible: audioPlayer.albumArtPath === ""
                    }
                }
            }

            // Track Info
            ColumnLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter
                spacing: Kirigami.Units.smallSpacing

                // Title
                Controls.Label {
                    Layout.fillWidth: true
                    text: audioPlayer.hasMetadata ? audioPlayer.title : (mediaPlayer.source.toString() !== "" ? i18n("Unknown Title") : "")
                    font.bold: true
                    font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.2
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
                }

                // Artist - Album
                Controls.Label {
                    Layout.fillWidth: true
                    text: {
                        if (!audioPlayer.hasMetadata && mediaPlayer.source.toString() === "") {
                            return "";
                        }
                        var artist = audioPlayer.hasMetadata ? audioPlayer.artist : i18n("Unknown Artist");
                        var album = audioPlayer.hasMetadata && audioPlayer.album !== "" ? audioPlayer.album : "";
                        if (album !== "") {
                            return artist + " - " + album;
                        }
                        return artist;
                    }
                    color: Kirigami.Theme.disabledTextColor
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
                }
            }

            // Spacer
            Item {
                Layout.fillHeight: true
            }

            // Seek Bar with Time Display
            ColumnLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing

                Controls.Slider {
                    id: seekSlider
                    Layout.fillWidth: true
                    from: 0
                    to: mediaPlayer.duration > 0 ? mediaPlayer.duration : 1
                    value: mediaPlayer.position
                    enabled: mediaPlayer.duration > 0

                    onMoved: {
                        mediaPlayer.position = value;
                    }
                }

                RowLayout {
                    Layout.fillWidth: true

                    Controls.Label {
                        text: formatTime(mediaPlayer.position)
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                        color: Kirigami.Theme.disabledTextColor
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    Controls.Label {
                        text: formatTime(mediaPlayer.duration)
                        font.pointSize: Kirigami.Theme.smallFont.pointSize
                        color: Kirigami.Theme.disabledTextColor
                    }
                }
            }

            // Error Message Toast
            Kirigami.InlineMessage {
                id: errorMessage
                Layout.fillWidth: true
                type: Kirigami.MessageType.Error
                showCloseButton: true
                visible: false

                onVisibleChanged: {
                    if (!visible) {
                        errorHideTimer.stop();
                    }
                }
            }

            // Control Buttons
            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: Kirigami.Units.largeSpacing

                // Play/Pause Button
                Controls.Button {
                    icon.name: mediaPlayer.playbackState === MediaPlayer.PlayingState ? "media-playback-pause" : "media-playback-start"
                    text: mediaPlayer.playbackState === MediaPlayer.PlayingState ? i18n("Pause") : i18n("Play")
                    enabled: mediaPlayer.source.toString() !== ""
                    onClicked: {
                        if (mediaPlayer.playbackState === MediaPlayer.PlayingState) {
                            mediaPlayer.pause();
                        } else {
                            mediaPlayer.play();
                        }
                    }
                }

                // Stop Button
                Controls.Button {
                    icon.name: "media-playback-stop"
                    text: i18n("Stop")
                    enabled: mediaPlayer.source.toString() !== ""
                    onClicked: {
                        mediaPlayer.stop();
                        mediaPlayer.position = 0;
                    }
                }
            }
        }
    }
}
