////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2023 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2023.10.19 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/emitter.hpp"
#include "pfs/filesystem.hpp"
#include "pfs/fmt.hpp"
#include "pfs/log.hpp"
#include "pfs/ionik/audio/wav_explorer.hpp"
#include <cstdlib>
#include <thread>

#if IONIK__QT5_GUI_ENABLED
#   include "WavSpectrum.hpp"
#   include <QGuiApplication>
#   include <QQmlApplicationEngine>
#   include <QQmlContext>
#endif

namespace fs = pfs::filesystem;

bool build_spectrum (fs::path const & au_path
    , pfs::emitter<ionik::audio::wav_spectrum const &> & spectrumCompleted
    , pfs::emitter<> & spectrumFailure)
{
    ionik::audio::wav_explorer explorer {au_path};
    ionik::audio::wav_spectrum_builder spectrum_builder {explorer};

    std::size_t chunk_count = 25;
    std::size_t frame_step = 1;    // step by every frame
    auto res = spectrum_builder(chunk_count, frame_step);

    if (res) {
        LOGD("", "Spectrum size: {}", res->data.size());

        switch (res->info.num_channels) {
            case 1:
                LOGD("", "Max sample: {}", res->max_frame.first);
                LOGD("", "Min sample: {}", res->min_frame.first);
                break;
            case 2:
                LOGD("", "Max samples: left={}, right={}"
                    , res->max_frame.first
                    , res->max_frame.second);
                LOGD("", "Min samples: left={}, right={}"
                    , res->min_frame.first
                    , res->min_frame.second);
                break;
            default:
                LOGE("", "unexpected state");
                break;
        }

        spectrumCompleted(std::move(*res));
        return true;
    }

    LOGE("", "{}", res.error().what());

    spectrumFailure();
    return false;
}

int main (int argc, char * argv[])
{
    if (argc < 2) {
        LOGE("", "WAV file expected as first argument");
        LOGE("", "Usage:\n\t{} WAV_FILE_PATH", argv[0]);
        return EXIT_FAILURE;
    }

    auto au_path = fs::utf8_decode(argv[1]);

    if (!(fs::exists(au_path) && fs::is_regular_file(au_path))) {
        LOGE("", "File not found or it is not a regular file: {}", au_path);
        return EXIT_FAILURE;
    }

    int exit_status = EXIT_SUCCESS;

#if IONIK__QT5_GUI_ENABLED
    QGuiApplication::setAttribute(Qt::AA_ShareOpenGLContexts, true);
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);

#if _MSC_VER
    QGuiApplication::setAttribute(Qt::AA_UseDesktopOpenGL, true);
#endif

    QGuiApplication app {argc, argv};
    QQmlApplicationEngine engine;
    WavSpectrum wavSpectrum;
    engine.rootContext()->setContextProperty("WavSpectrum", & wavSpectrum);

    engine.load("qrc:/MainWindow.qml");

    if (engine.rootObjects().isEmpty())
        exit_status = EXIT_FAILURE;
#else
#endif

    pfs::emitter<ionik::audio::wav_spectrum const &> spectrumCompleted;
    pfs::emitter<> spectrumFailure;

    spectrumCompleted.connect([& wavSpectrum] (ionik::audio::wav_spectrum const & spectrum) {
        wavSpectrum.setSpectrum(spectrum);
    });

    spectrumFailure.connect([& wavSpectrum] () {
        wavSpectrum.setFailure();
    });

    std::thread buildSpectrumThread {
          build_spectrum
        , au_path
        , std::ref(spectrumCompleted)
        , std::ref(spectrumFailure)
    };

#if IONIK__QT5_GUI_ENABLED
    if (exit_status == EXIT_SUCCESS) {
        if (app.exec() != 0)
            exit_status = EXIT_FAILURE;
    }
#else
#endif
    buildSpectrumThread.join();

    return exit_status;
}
