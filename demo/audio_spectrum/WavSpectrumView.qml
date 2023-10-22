////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2023 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2023.10.20 Initial version.
////////////////////////////////////////////////////////////////////////////////
import QtQuick 2.8

Canvas {
    property real spaceFactor: 0.5
    property var wavSpectrum: null
    property color playbackedColor: "#38A88F"
    property color unplaybackedColor: "#86D6C5"
    property real playbacked: 0

    QtObject {
        id: local
        property int lastPlaybackedFrameIndex: 0
    }

    onPlaybackedChanged: {
        var lastPlaybackedFrameIndex = this.playbacked * wavSpectrum.frameCount;

        if (lastPlaybackedFrameIndex != local.lastPlaybackedFrameIndex) {
            local.lastPlaybackedFrameIndex = lastPlaybackedFrameIndex;
            this.requestPaint();
        }
    }

    function frameWidth ()
    {
        return wavSpectrum != null && wavSpectrum.frameCount > 0
            ? this.width / (wavSpectrum.frameCount
                + this.spaceFactor * (wavSpectrum.frameCount - 1))
            : 0;
    }

    function paintMonoSpectrum (ctx)
    {
        var frameWidth = this.frameWidth();
        var spaceWidth = this.spaceFactor * frameWidth;

        ctx.lineWidth = frameWidth;
        ctx.lineCap = "round"

        var currentX = frameWidth / 2;
        var baseLineY = this.height;
        var scaleFactor = this.height / wavSpectrum.maxSampleValue();

        var heightCorrection = 0;

        if (ctx.lineCap == "round")
            heightCorrection = frameWidth / 2;

        var limits = [[playbackedColor, 0, local.lastPlaybackedFrameIndex]
            , [unplaybackedColor, local.lastPlaybackedFrameIndex, wavSpectrum.frameCount]];

        for (var k = 0; k < 2; k++) {
            ctx.strokeStyle = limits[k][0];
            ctx.beginPath();

            var from = limits[k][1];
            var to = limits[k][2];

            for (var i = from; i < to; i++) {
                var left = scaleFactor * Math.abs(wavSpectrum.leftAt(i));

                ctx.moveTo(currentX, baseLineY);
                ctx.lineTo(currentX, baseLineY - left + heightCorrection);

                currentX += frameWidth + spaceWidth;
            }

            ctx.stroke();
        }
    }

    function paintStereoSpectrum (ctx)
    {
        var frameWidth = this.frameWidth();
        var spaceWidth = this.spaceFactor * frameWidth;

        ctx.lineWidth = frameWidth;
        ctx.lineCap = "round"
        ctx.strokeStyle = playbackedColor

        var currentX = frameWidth / 2;
        var baseLineY = this.height / 2;
        var scaleFactor = (this.height / 2) / wavSpectrum.maxSampleValue();

        var heightCorrection = 0;

        if (ctx.lineCap == "round")
            heightCorrection = frameWidth / 2;

        var limits = [[playbackedColor, 0, local.lastPlaybackedFrameIndex]
            , [unplaybackedColor, local.lastPlaybackedFrameIndex, wavSpectrum.frameCount]];

        for (var k = 0; k < 2; k++) {
            ctx.strokeStyle = limits[k][0];
            ctx.beginPath();

            var from = limits[k][1];
            var to = limits[k][2];

            for (var i = from; i < to; i++) {
                var left  = scaleFactor * Math.abs(wavSpectrum.leftAt(i));
                var right = scaleFactor * Math.abs(wavSpectrum.rightAt(i));

                ctx.moveTo(currentX, baseLineY);
                ctx.lineTo(currentX, baseLineY - left + heightCorrection);

                ctx.moveTo(currentX, baseLineY);
                ctx.lineTo(currentX, baseLineY + right - heightCorrection);

                currentX += frameWidth + spaceWidth;
            }

            ctx.stroke();
        }
    }

    onPaint: {
        if (this.width <= 0)
            return;

        if (this.height <= 0)
            return;

        if (wavSpectrum == null)
            return;

        if (wavSpectrum.frameCount <= 0)
            return;

        var ctx = getContext('2d');

        if (ctx == null)
            return;

        ctx.clearRect(0, 0, this.width, this.height);

        if (wavSpectrum.stereo) {
            paintStereoSpectrum(ctx);
        } else {
            paintMonoSpectrum(ctx);
        }
    }
}
