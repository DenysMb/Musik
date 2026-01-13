// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Musik Authors

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import QtQuick.Dialogs
import QtQuick.Effects
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

    // Track if a file is loaded
    readonly property bool hasFile: mediaPlayer.source.toString() !== ""

    // Playback mode properties
    property bool shuffleEnabled: false
    property int repeatMode: 0 // 0=Off, 1=One, 2=All

    // Supported audio extensions for drag-and-drop validation
    readonly property var audioExtensions: [".mp3", ".flac", ".ogg", ".wav", ".m4a", ".aac", ".wma", ".opus"]

    // Check if a file has a supported audio extension
    function isAudioFile(filePath) {
        var path = filePath.toString().toLowerCase();
        for (var i = 0; i < audioExtensions.length; i++) {
            if (path.endsWith(audioExtensions[i])) {
                return true;
            }
        }
        return false;
    }

    // Load and play an audio file
    function openFile(fileUrl) {
        audioPlayer.loadFile(fileUrl);
        mediaPlayer.source = fileUrl;
        mediaPlayer.play();
    }

    // Play a track from the playlist by URL
    function playTrack(url) {
        if (!url || url.toString() === "")
            return;
        audioPlayer.loadFile(url);
        mediaPlayer.source = url;
        mediaPlayer.play();
    }

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

    // PlaylistModel C++ backend for playlist management
    PlaylistModel {
        id: playlistModel

        onErrorOccurred: function (message) {
            showError(message);
        }
    }

    // Playlist Drawer
    Kirigami.OverlayDrawer {
        id: playlistDrawer
        edge: Qt.RightEdge
        modal: true
        width: Math.min(root.width * 0.85, 320)

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // Header row
            RowLayout {
                Layout.fillWidth: true
                Layout.margins: Kirigami.Units.largeSpacing

                Kirigami.Heading {
                    text: i18n("Playlist")
                    level: 2
                    Layout.fillWidth: true
                }

                Controls.ToolButton {
                    icon.name: "edit-clear-all"
                    display: Controls.AbstractButton.IconOnly
                    text: i18n("Clear playlist")
                    onClicked: playlistModel.clear()
                    enabled: playlistModel.count > 0

                    Controls.ToolTip.text: text
                    Controls.ToolTip.visible: hovered
                    Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
                }
            }

            Kirigami.Separator {
                Layout.fillWidth: true
            }

            // Playlist ListView with track delegates
            ListView {
                id: playlistView
                Layout.fillWidth: true
                Layout.fillHeight: true
                model: playlistModel
                clip: true
                reuseItems: true

                delegate: Kirigami.SwipeListItem {
                    id: trackDelegate
                    width: ListView.view.width
                    highlighted: index === playlistModel.currentIndex

                    onClicked: {
                        playlistModel.setCurrentIndex(index);
                        playTrack(playlistModel.urlAt(index));
                    }

                    contentItem: RowLayout {
                        spacing: Kirigami.Units.smallSpacing

                        // Album art thumbnail
                        Rectangle {
                            Layout.preferredWidth: 48
                            Layout.preferredHeight: 48
                            color: Kirigami.Theme.backgroundColor
                            radius: 4

                            Image {
                                anchors.fill: parent
                                anchors.margins: 1
                                source: model.albumArt || ""
                                fillMode: Image.PreserveAspectCrop
                                visible: model.albumArt !== ""
                            }

                            Kirigami.Icon {
                                anchors.centerIn: parent
                                width: 24
                                height: 24
                                source: "media-optical-audio"
                                visible: model.albumArt === ""
                                opacity: 0.5
                            }
                        }

                        // Track info
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            Controls.Label {
                                Layout.fillWidth: true
                                text: model.title || i18n("Unknown Title")
                                elide: Text.ElideRight
                                font.bold: trackDelegate.highlighted
                            }

                            Controls.Label {
                                Layout.fillWidth: true
                                text: model.artist || i18n("Unknown Artist")
                                elide: Text.ElideRight
                                color: Kirigami.Theme.disabledTextColor
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                            }
                        }
                    }
                }
            }
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
            openFile(selectedFile);
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
                text: i18n("Playlist")
                icon.name: "view-media-playlist"
                onTriggered: playlistDrawer.open()
            },
            Kirigami.Action {
                text: i18n("Open")
                icon.name: "document-open"
                onTriggered: fileDialog.open()
            }
        ]

        // Blurred album art background
        Item {
            id: backgroundContainer
            anchors.fill: parent
            visible: audioPlayer.albumArtPath !== ""
            z: -1

            Image {
                id: backgroundImage
                anchors.centerIn: parent
                width: Math.max(parent.width, parent.height)
                height: width
                source: audioPlayer.albumArtPath
                fillMode: Image.PreserveAspectCrop
                visible: false
            }

            MultiEffect {
                anchors.fill: parent
                source: backgroundImage
                blurEnabled: true
                blurMax: 64
                blur: 1.0
                opacity: 0.5
            }
        }

        // Drag and drop area
        DropArea {
            id: dropArea
            anchors.fill: parent

            onEntered: function (drag) {
                if (drag.hasUrls) {
                    drag.accepted = true;
                } else {
                    drag.accepted = false;
                }
            }

            onDropped: function (drop) {
                if (drop.hasUrls && drop.urls.length > 0) {
                    var fileUrl = drop.urls[0];
                    if (isAudioFile(fileUrl)) {
                        openFile(fileUrl);
                    } else {
                        showError(i18n("Unsupported file format"));
                    }
                }
            }

            // Visual feedback when dragging
            Rectangle {
                anchors.fill: parent
                color: Kirigami.Theme.highlightColor
                opacity: dropArea.containsDrag ? 0.2 : 0
                radius: Kirigami.Units.smallSpacing

                Behavior on opacity {
                    NumberAnimation {
                        duration: 150
                    }
                }
            }

            // Empty State Placeholder
            ColumnLayout {
                anchors.centerIn: parent
                spacing: Kirigami.Units.largeSpacing
                visible: !root.hasFile

                Kirigami.Icon {
                    Layout.alignment: Qt.AlignHCenter
                    width: 128
                    height: 128
                    source: "folder-music-symbolic"
                    opacity: 0.5
                }

                Kirigami.Heading {
                    Layout.alignment: Qt.AlignHCenter
                    text: i18n("No audio file loaded")
                    level: 2
                    opacity: 0.7
                }

                Controls.Label {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.maximumWidth: 300
                    text: i18n("Click \"Open\" to select a file or drag and drop an audio file here to start listening")
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                    color: Kirigami.Theme.disabledTextColor
                }
            }

            // Player UI (visible when file is loaded)
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Kirigami.Units.largeSpacing
                spacing: Kirigami.Units.largeSpacing
                visible: root.hasFile

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
                            fillMode: Image.PreserveAspectCrop
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
                        text: audioPlayer.hasMetadata ? audioPlayer.title : i18n("Unknown Title")
                        font.bold: true
                        font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.2
                        horizontalAlignment: Text.AlignHCenter
                        elide: Text.ElideRight
                    }

                    // Artist - Album
                    Controls.Label {
                        Layout.fillWidth: true
                        text: {
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
                    spacing: Kirigami.Units.smallSpacing

                    // Previous Track Button
                    Controls.ToolButton {
                        icon.name: "media-skip-backward"
                        enabled: playlistModel.count > 0
                        onClicked: {
                            var prevIdx = playlistModel.previousIndex();
                            if (prevIdx >= 0) {
                                playlistModel.setCurrentIndex(prevIdx);
                                playTrack(playlistModel.urlAt(prevIdx));
                            }
                        }

                        Controls.ToolTip.text: i18n("Previous track")
                        Controls.ToolTip.visible: hovered
                        Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
                    }

                    // Play/Pause Button
                    Controls.Button {
                        icon.name: mediaPlayer.playbackState === MediaPlayer.PlayingState ? "media-playback-pause" : "media-playback-start"
                        text: mediaPlayer.playbackState === MediaPlayer.PlayingState ? i18n("Pause") : i18n("Play")
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
                        onClicked: {
                            mediaPlayer.stop();
                            mediaPlayer.position = 0;
                        }
                    }

                    // Next Track Button
                    Controls.ToolButton {
                        icon.name: "media-skip-forward"
                        enabled: playlistModel.count > 0
                        onClicked: {
                            var nextIdx = playlistModel.nextIndex(shuffleEnabled, repeatMode);
                            if (nextIdx >= 0) {
                                playlistModel.setCurrentIndex(nextIdx);
                                playTrack(playlistModel.urlAt(nextIdx));
                            }
                        }

                        Controls.ToolTip.text: i18n("Next track")
                        Controls.ToolTip.visible: hovered
                        Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
                    }
                }
            }
        }
    }
}
