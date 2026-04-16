// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Musik Authors

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import QtQuick.Dialogs
import QtQuick.Effects
import QtMultimedia
import org.kde.kirigami as Kirigami
import org.kde.coreaddons
import org.koderoots.musik

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

    // Control auto-play when adding files via dialog
    property bool shouldAutoPlay: true

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

    // AudioOutput for volume control
    AudioOutput {
        id: audioOutput
        volume: Settings.muted ? 0.0 : Settings.volume / 100.0
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
        handleVisible: false
        width: Math.min(root.width * 0.85, 320)

        ColumnLayout {
            anchors.fill: parent
            // spacing: Kirigami.Units.largeSpacing

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
                    text: i18n("Clear playlist")
                    onClicked: {
                        playlistModel.clear();
                        mediaPlayer.stop();
                        mediaPlayer.source = "";
                        audioPlayer.clearMetadata();
                    }
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
                visible: playlistModel.count > 0

                delegate: Controls.ItemDelegate {
                    id: trackDelegate
                    width: ListView.view.width
                    highlighted: index === playlistModel.currentIndex
                    padding: Kirigami.Units.mediumSpacing

                    onClicked: {
                        playlistModel.currentIndex = index;
                        playTrack(playlistModel.urlAt(index));
                    }

                    background: Rectangle {
                        color: trackDelegate.highlighted ? Kirigami.Theme.highlightColor : (trackDelegate.hovered ? Kirigami.Theme.hoverColor : "transparent")
                        radius: Kirigami.Units.smallSpacing
                    }

                    contentItem: RowLayout {
                        spacing: Kirigami.Units.smallSpacing

                        // Album art thumbnail
                        Rectangle {
                            Layout.preferredWidth: Layout.preferredHeight
                            Layout.preferredHeight: parent.height
                            color: Kirigami.Theme.backgroundColor

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
                                color: trackDelegate.highlighted ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.textColor
                            }

                            Controls.Label {
                                Layout.fillWidth: true
                                text: model.artist || i18n("Unknown Artist")
                                elide: Text.ElideRight
                                color: Kirigami.Theme.disabledTextColor
                                font.pointSize: Kirigami.Theme.smallFont.pointSize
                            }
                        }

                        // Remove button
                        Controls.ToolButton {
                            icon.name: "list-remove"
                            onClicked: {
                                var removingCurrent = (index === playlistModel.currentIndex);
                                var wasPlaying = (mediaPlayer.playbackState === MediaPlayer.PlayingState);
                                var nextUrl = "";

                                // If removing current track, prepare next track
                                if (removingCurrent && playlistModel.count > 1) {
                                    var nextIdx = (index + 1 < playlistModel.count) ? index + 1 : index - 1;
                                    nextUrl = playlistModel.urlAt(nextIdx);
                                }

                                // Remove the track
                                playlistModel.removeTrack(index);

                                // Handle playback after removal
                                if (removingCurrent) {
                                    if (playlistModel.count > 0 && nextUrl !== "") {
                                        // Play next track if we were playing
                                        if (wasPlaying) {
                                            playTrack(nextUrl);
                                        } else {
                                            // Just load metadata without playing
                                            audioPlayer.loadFile(nextUrl);
                                        }
                                    } else {
                                        // Playlist is now empty - stop playback and clear metadata
                                        mediaPlayer.stop();
                                        mediaPlayer.source = "";
                                        audioPlayer.clearMetadata();
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Kirigami.Separator {
                Layout.fillWidth: true
                visible: playlistModel.count > 0
            }

            // Add Files button (visible only when playlist has tracks)
            Controls.Button {
                Layout.fillWidth: true
                Layout.margins: Kirigami.Units.largeSpacing
                text: i18n("Add Tracks")
                icon.name: "list-add"
                visible: playlistModel.count > 0
                onClicked: {
                    shouldAutoPlay = false;
                    fileDialog.open();
                }
            }

            // Empty state when no tracks in playlist
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: playlistModel.count === 0
                spacing: Kirigami.Units.largeSpacing

                Item {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                }

                Kirigami.Icon {
                    Layout.alignment: Qt.AlignHCenter
                    source: "view-media-playlist"
                    width: 64
                    height: 64
                    opacity: 0.5
                }

                Kirigami.Heading {
                    Layout.alignment: Qt.AlignHCenter
                    text: i18n("No tracks in playlist")
                    level: 3
                    opacity: 0.7
                }

                Controls.Button {
                    Layout.alignment: Qt.AlignHCenter
                    text: i18n("Open")
                    icon.name: "document-open"
                    onClicked: fileDialog.open()
                }

                Item {
                    Layout.fillHeight: true
                }
            }
        }
    }

    // MediaPlayer for audio playback
    MediaPlayer {
        id: mediaPlayer
        audioOutput: audioOutput

        onPlaybackStateChanged: {
            // Auto-play next track when current track ends naturally
            if (mediaPlayer.playbackState === MediaPlayer.StoppedState && mediaPlayer.position >= mediaPlayer.duration - 100 && mediaPlayer.duration > 0) {
                // Only auto-advance if playlist has tracks
                if (playlistModel.count > 0) {
                    // Repeat One: replay same track
                    if (repeatMode === 1) {
                        mediaPlayer.position = 0;
                        mediaPlayer.play();
                        return;
                    }

                    // Repeat Off/All: get next track (handles wrapping for repeat all)
                    var nextIdx = playlistModel.nextIndex(shuffleEnabled, repeatMode);
                    if (nextIdx >= 0) {
                        playlistModel.currentIndex = nextIdx;
                        playTrack(playlistModel.urlAt(nextIdx));
                    } else {
                        // End of playlist (repeat off) - reset position
                        mediaPlayer.position = 0;
                    }
                } else {
                    // No playlist - just reset position
                    mediaPlayer.position = 0;
                }
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
        title: i18n("Open Audio Files")
        fileMode: FileDialog.OpenFiles
        nameFilters: [i18n("Audio Files") + " (*.mp3 *.flac *.ogg *.wav *.m4a *.aac *.wma *.opus)", i18n("All Files") + " (*)"]
        onAccepted: {
            var wasEmpty = (playlistModel.count === 0);
            var wasPlaying = (mediaPlayer.playbackState === MediaPlayer.PlayingState);

            // Add all selected files to playlist
            playlistModel.addTracks(selectedFiles);

            // Auto-play if shouldAutoPlay is enabled
            if (shouldAutoPlay) {
                if (selectedFiles.length === 1 && playlistModel.count > 0) {
                    playlistModel.currentIndex = playlistModel.count - 1;
                    playTrack(playlistModel.urlAt(playlistModel.currentIndex));
                } else if (wasEmpty && playlistModel.count > 0) {
                    playlistModel.currentIndex = 0;
                    playTrack(playlistModel.urlAt(0));
                }
            }
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
                onTriggered: {
                    shouldAutoPlay = true;
                    fileDialog.open();
                }
            },
            Kirigami.Action {
                text: i18nc("@action", "About")
                icon.name: "help-about"
                onTriggered: root.pageStack.layers.push(aboutPage)
            }
        ]

        Component {
            id: aboutPage
            Kirigami.AboutPage {
                aboutData: AboutData
            }
        }

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
                    var wasEmpty = (playlistModel.count === 0);
                    var audioUrls = [];

                    // Filter for audio files
                    for (var i = 0; i < drop.urls.length; i++) {
                        if (isAudioFile(drop.urls[i])) {
                            audioUrls.push(drop.urls[i]);
                        }
                    }

                    if (audioUrls.length > 0) {
                        // Add all audio files to playlist
                        playlistModel.addTracks(audioUrls);

                        // Auto-play if shouldAutoPlay is enabled
                        if (shouldAutoPlay) {
                            if (audioUrls.length === 1 && playlistModel.count > 0) {
                                playlistModel.currentIndex = playlistModel.count - 1;
                                playTrack(playlistModel.urlAt(playlistModel.currentIndex));
                            } else if (wasEmpty && playlistModel.count > 0) {
                                playlistModel.currentIndex = 0;
                                playTrack(playlistModel.urlAt(0));
                            }
                        }
                    } else {
                        showError(i18n("No supported audio files found"));
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

                    // Shuffle Button
                    Controls.ToolButton {
                        icon.name: "media-playlist-shuffle"
                        checkable: true
                        checked: shuffleEnabled
                        onCheckedChanged: shuffleEnabled = checked

                        Controls.ToolTip.text: shuffleEnabled ? i18n("Shuffle: On") : i18n("Shuffle: Off")
                        Controls.ToolTip.visible: hovered
                        Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
                    }

                    // Previous Track Button
                    Controls.ToolButton {
                        icon.name: "media-skip-backward"
                        enabled: playlistModel.count > 1
                        onClicked: {
                            var prevIdx = playlistModel.previousIndex();
                            if (prevIdx >= 0) {
                                playlistModel.currentIndex = prevIdx;
                                playTrack(playlistModel.urlAt(prevIdx));
                            }
                        }

                        Controls.ToolTip.text: i18n("Previous track")
                        Controls.ToolTip.visible: hovered
                        Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
                    }

                    // Play/Pause Button
                    Controls.ToolButton {
                        icon.name: mediaPlayer.playbackState === MediaPlayer.PlayingState ? "media-playback-pause" : "media-playback-start"
                        onClicked: {
                            if (mediaPlayer.playbackState === MediaPlayer.PlayingState) {
                                mediaPlayer.pause();
                            } else {
                                mediaPlayer.play();
                            }
                        }

                        Controls.ToolTip.text: mediaPlayer.playbackState === MediaPlayer.PlayingState ? i18n("Pause") : i18n("Play")
                        Controls.ToolTip.visible: hovered
                        Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
                    }

                    // Stop Button
                    // Controls.ToolButton {
                    //     icon.name: "media-playback-stop"
                    //     onClicked: {
                    //         mediaPlayer.stop();
                    //         mediaPlayer.position = 0;
                    //     }

                    //     Controls.ToolTip.text: i18n("Stop")
                    //     Controls.ToolTip.visible: hovered
                    //     Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
                    // }

                    // Next Track Button
                    Controls.ToolButton {
                        icon.name: "media-skip-forward"
                        enabled: playlistModel.count > 1
                        onClicked: {
                            var nextIdx = playlistModel.nextIndex(shuffleEnabled, repeatMode);
                            if (nextIdx >= 0) {
                                playlistModel.currentIndex = nextIdx;
                                playTrack(playlistModel.urlAt(nextIdx));
                            }
                        }

                        Controls.ToolTip.text: i18n("Next track")
                        Controls.ToolTip.visible: hovered
                        Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
                    }

                    // Repeat Button
                    Controls.ToolButton {
                        icon.name: repeatMode === 1 ? "media-playlist-repeat-song" : "media-playlist-repeat"
                        checkable: true
                        checked: repeatMode !== 0
                        opacity: repeatMode === 0 ? 0.5 : 1.0

                        onClicked: {
                            // Cycle: Off(0) -> All(2) -> One(1) -> Off(0)
                            if (repeatMode === 0)
                                repeatMode = 2;
                            else if (repeatMode === 2)
                                repeatMode = 1;
                            else
                                repeatMode = 0;
                        }

                    Controls.ToolTip.text: repeatMode === 0 ? i18n("Repeat: Off") : repeatMode === 1 ? i18n("Repeat: One") : i18n("Repeat: All")
                    Controls.ToolTip.visible: hovered
                    Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
                    }

                    // Mute Button
                    Controls.ToolButton {
                        icon.name: Settings.muted ? "audio-volume-muted" : "audio-volume-high"
                        checkable: true
                        checked: Settings.muted
                        onCheckedChanged: Settings.muted = checked

                        Controls.ToolTip.text: Settings.muted ? i18n("Unmute") : i18n("Mute")
                        Controls.ToolTip.visible: hovered
                        Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
                    }

                    // Volume Slider
                    Controls.Slider {
                        id: volumeSlider
                        Layout.preferredWidth: 100
                        from: 0
                        to: 100
                        value: Settings.volume
                        stepSize: 1

                        onMoved: {
                            Settings.volume = Math.round(value);
                        }

                        Controls.ToolTip.text: i18n("Volume: %1%", Settings.volume)
                        Controls.ToolTip.visible: hovered
                        Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
                    }
                }
            }
        }
    }
}
