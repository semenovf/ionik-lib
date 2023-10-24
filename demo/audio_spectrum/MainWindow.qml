////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2023 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2023.10.19 Initial version.
////////////////////////////////////////////////////////////////////////////////
import QtQuick 2.8
import QtQuick.Controls 2.8
import QtQuick.Window 2.2
// Audio.notifyInterval requires 5.9
import QtMultimedia 5.9

Window {
    x: 100
    y: 100
    width: 640
    height: 480
    visible: true

    BusyIndicator {
        id: busyIndicator
        running: true
        anchors.centerIn: parent
    }

    WavSpectrumView {
        id: spectrumView
        visible: !busyIndicator.running
        wavSpectrum: WavSpectrum
        spaceFactor: 0.5
        anchors.fill: parent

        onWidthChanged: {
            spectrumView.requestPaint();
        }

        onHeightChanged: {
            spectrumView.requestPaint();
        }

        // onFrameCountChanged: {
        //     spectrumView.requestPaint();
        // }

        Connections {
            target: WavSpectrum

            function onSpectrumChanged () {
                busyIndicator.running = false;

                if (WavSpectrum.good)
                    spectrumView.requestPaint();
            }
        }
    }

    RoundButton {
        anchors.centerIn: parent
        visible: !busyIndicator.running && audioPlayer.status == Audio.NoMedia
        icon.width: 100
        icon.height: 100
        icon.color: "#018786" // "transparent"
        icon.source: "qrc:/Icons/Playback.svg"

        background: Rectangle {
            color: "transparent"
        }

        onClicked: {
            audioPlayer.source = WavSpectrum.source;
            console.log("-- PLAY: " + WavSpectrum.source);
            audioPlayer.play();
        }
    }

    Audio {
        id: audioPlayer
        autoLoad: false
        autoPlay: false
        notifyInterval: 100 // millis

        onPositionChanged: {
            if (this.duration > 0)
                spectrumView.playbacked = this.position / this.duration;
        }
    }
}
