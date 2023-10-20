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
}
