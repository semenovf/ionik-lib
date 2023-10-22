////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2023 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2023.10.12 Initial version.
////////////////////////////////////////////////////////////////////////////////
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "pfs/filesystem.hpp"
#include "pfs/ionik/audio/wav_explorer.hpp"

// Source of test audio files
// https://www.mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/Samples.html
// https://mauvecloud.net/sounds/index.html

namespace fs = pfs::filesystem;

TEST_CASE("wav_explorer") {
    struct {
        char const * filename;
        ionik::audio::wav_info info;
    } test_data[] = {
        // https://mauvecloud.net/sounds/pcm0808m.wav
        // PCM (uncompressed) 8 bit Mono 8000 Hz
        { "pcm0808m.wav"
            , {
                  pfs::endian::little // byte_order
                ,       1 // audio_format
                ,       1 // num_channels: Mono = 1, Stereo = 2, etc.
                ,    8000 // sample_rate: 8000, 44100, etc.
                ,       8 // sample_size: 8 bits = 8, 16 bits = 16, etc.
                ,    8000 // byte_rate
                ,   53499 // sample_count
                ,   53499 // frame_count
                , 6687375 // duration
                , {0x64617461, 53499, 44} // data
                // Extra parameters
                , {}
            }
        }
        // https://www.mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/Samples/SoundCardAttrition/stereol.wav
        // A standard 16-bit stereo WAVE file, but with a long header, 22050 Hz, 16-bit, 1.32 s
        , { "stereol.wav"
              , {
                  pfs::endian::little // byte_order
                ,       1 // audio_format
                ,       2 // num_channels: Mono = 1, Stereo = 2, etc.
                ,   22050 // sample_rate: 8000, 44100, etc.
                ,      16 // sample_size: 8 bits = 8, 16 bits = 16, etc.
                ,   88200 // byte_rate
                ,   58032 // sample_count
                ,   29016 // frame_count
                , 1315918 // duration
                , {0x64617461, 116064, 2136} // data
                // Extra parameters
                , {
                      {0x5045414b, 0, 0} // "PEAK" TODO need to check last two fields
                    , {0x63756520, 0, 0} // "cue "
                    , {0x4c495354, 0, 0} // "LIST"
                }
            }
        }

        // https://www.mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/Samples/AFsp/M1F1-uint8-AFsp.wav
        // WAVE file, stereo unsigned 8-bit data
        , { "M1F1-uint8-AFsp.wav"
              , {
                  pfs::endian::little // byte_order
                ,       1 // audio_format
                ,       2 // num_channels
                ,    8000 // sample_rate
                ,       8 // sample_size: 8 bits = 8, 16 bits = 16, etc.
                ,   16000 // byte_rate
                ,   46986 // sample_count
                ,   23493 // frame_count
                , 2936625 // duration
                , {0x64617461, 46986, 44} // data
                , {}
            }
        }

        // https://www.mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/Samples/AFsp/M1F1-Alaw-AFsp.wav
        // WAVE file, stereo A-law data
        , { "M1F1-Alaw-AFsp.wav"
              , {
                  pfs::endian::little // byte_order
                ,       6 // audio_format
                ,       2 // num_channels
                ,    8000 // sample_rate
                ,       8 // sample_size: 8 bits = 8, 16 bits = 16, etc.
                ,   16000 // byte_rate
                ,   46986 // sample_count
                ,   23493 // frame_count
                , 2936625 // duration
                , {0x64617461, 46986, 58} // data
                , { {0x66616374, 0, 0} }
            }
        }
    };

    for (auto const & elem: test_data) {
        ionik::audio::wav_explorer wav_explorer{ fs::current_path()
            / PFS__LITERAL_PATH("data")
            / PFS__LITERAL_PATH("au")
            / fs::utf8_decode(elem.filename)
        };

        auto expected = wav_explorer.read_header();

        if (!expected) {
            fmt::println(stderr, "ERROR: {}", expected.error().what());
        }

        REQUIRE_EQ(expected.has_value(), true);
        CHECK_EQ(expected->byte_order, elem.info.byte_order);
        CHECK_EQ(expected->audio_format, elem.info.audio_format);
        CHECK_EQ(expected->num_channels, elem.info.num_channels);
        CHECK_EQ(expected->sample_rate, elem.info.sample_rate);
        CHECK_EQ(expected->sample_size, elem.info.sample_size);
        CHECK_EQ(expected->byte_rate, elem.info.byte_rate);
        CHECK_EQ(expected->sample_count, elem.info.sample_count);
        CHECK_EQ(expected->frame_count, elem.info.frame_count);
        CHECK_EQ(expected->duration, elem.info.duration);
        CHECK_EQ(expected->data.id, elem.info.data.id);
        CHECK_EQ(expected->data.size, elem.info.data.size);
        CHECK_EQ(expected->data.start_offset, elem.info.data.start_offset);

        REQUIRE_EQ(expected->extra.size(), elem.info.extra.size());

        for (int i = 0; i < expected->extra.size(); i++) {
            CHECK_EQ(expected->extra[i].id, elem.info.extra[i].id);
        }

        fmt::println("Duration ({}): {}", elem.filename
            , ionik::audio::stringify_duration(expected->duration
                , ionik::audio::duration_precision::milliseconds));
    }
}

#if IONIK__QT5_MULTIMEDIA_ENABLED

#include "pfs/qt_compat.hpp"
#include <QAudioDecoder>
#include <QCoreApplication>

TEST_CASE("QAudioDecoder") {
    auto au_path = fs::current_path()
        / PFS__LITERAL_PATH("data")
        / PFS__LITERAL_PATH("au")
        / PFS__LITERAL_PATH("stereol.wav");
    ionik::audio::wav_explorer wav_explorer { au_path };

    auto res = wav_explorer.read_header();

    if (!res) {
        fmt::println(stderr, "ERROR: {}", res.error().what());
    }

    REQUIRE_EQ(res.has_value(), true);
    REQUIRE_EQ(res->audio_format, 1); // PCM

    // Make sure the data we receive is in correct PCM format.
    // Our wav file writer only supports SignedInt sample type.
    QAudioFormat audioFormat;
    audioFormat.setCodec("audio/pcm");
    audioFormat.setChannelCount(res->num_channels);
    audioFormat.setSampleRate(res->sample_rate);
    audioFormat.setSampleSize(res->sample_size);
    audioFormat.setSampleType(res->sample_size == 8
        ? QAudioFormat::UnSignedInt
        : res->sample_size == 16
            ? QAudioFormat::SignedInt
            : QAudioFormat::Float);
    audioFormat.setByteOrder(res->byte_order == pfs::endian::little
        ? QAudioFormat::LittleEndian : QAudioFormat::BigEndian);

    int argc = 0;
    QCoreApplication app(argc, nullptr);

    QAudioDecoder au_decoder;
    std::uint32_t totalSampleCount = 0;
    std::uint32_t totalFrameCount = 0;
    std::uint64_t totalDuration = 0;

    au_decoder.connect(& au_decoder, & QAudioDecoder::finished, [& totalSampleCount, & totalFrameCount, & totalDuration] {
        fmt::println("{:=>80}", "");
        fmt::println("Decoding finished successfully");
        fmt::println("Total Sample Count: {}", totalSampleCount);
        fmt::println("Total Frame Count : {}", totalFrameCount);
        fmt::println("Total Diration    : {} microseconds", totalDuration);
        CHECK_EQ(totalSampleCount, 58032);
        CHECK_EQ(totalFrameCount, 29016);
        QCoreApplication::quit();
    });

    au_decoder.connect(& au_decoder, static_cast<void (QAudioDecoder::*)(QAudioDecoder::Error)>(& QAudioDecoder::error)
        , [& au_decoder] (QAudioDecoder::Error) {
        fmt::println("Decoding failure: {}", au_decoder.errorString());
        QCoreApplication::quit();
    });

    au_decoder.connect(& au_decoder, & QAudioDecoder::bufferReady
        , [& au_decoder, & totalSampleCount, & totalFrameCount, & totalDuration] {
        QAudioBuffer au_buffer = au_decoder.read();
        auto audioFormat = au_buffer.format();
        totalSampleCount += au_buffer.sampleCount();
        totalFrameCount  += au_buffer.frameCount();
        totalDuration    += au_buffer.duration();

        fmt::println("-- Buffer ready: samples={}, frames={}, duration={} microseconds"
            , au_buffer.sampleCount(), au_buffer.frameCount(), au_buffer.duration());

        auto frames = au_buffer.constData<QAudioBuffer::S16S>();
        auto last_frame = frames[au_buffer.frameCount() - 1];
        fmt::println("LAST FRAME: left: {:>6}, right: {:>6}", last_frame.left
            , last_frame.right);
    });

    au_decoder.connect(& au_decoder, & QAudioDecoder::stateChanged
        , [] (QAudioDecoder::State state) {
        fmt::println("{:=>80}", "");
        fmt::println("-- State changed: {}", static_cast<int>(state));
    });

    au_decoder.connect(& au_decoder, & QAudioDecoder::positionChanged
        , [] (std::int64_t /*position*/) {
    });

    au_decoder.connect(& au_decoder, & QAudioDecoder::durationChanged
        , [] (std::int64_t duration) {

        // Decoder stopped
        if (duration < 0)
            return;

        auto hours = duration / 1000 / 60 / 24;
        auto minutes = (duration - hours * 1000 * 60 * 24) / 1000 / 60;
        auto seconds = (duration - hours * 1000 * 60 * 24 - minutes * 1000 * 60) / 1000;
        auto millis = duration - hours * 1000 * 60 * 24 - minutes * 1000 * 60 - seconds * 1000;

        if (hours > 0) {
            fmt::println("-- Duration: {:02}:{:02}:{:02}.{:03} ({} millis)"
                , hours, minutes, seconds, millis, duration);
        } else {
            fmt::println("-- Duration: {:02}:{:02}.{:03} ({} millis)"
                , minutes, seconds, millis, duration);
        }
    });

    // NOTE: Not all WAV files properly decoded by QAudioDecoder. In such cases
    // decoding stopped by error:
    //      Decoding failure: Internal data stream error.
    // But if set audio format by setAudioFormat(audioFormat), data processed
    // successfully.

    au_decoder.setAudioFormat(audioFormat);
    au_decoder.setSourceFilename(QString::fromStdString(pfs::filesystem::utf8_encode(au_path)));
    au_decoder.start();

    app.exec();
}
#endif // IONIK__QT5_MULTIMEDIA_ENABLED

TEST_CASE("ionik decoder") {
    auto au_path = fs::current_path()
        / PFS__LITERAL_PATH("data")
        / PFS__LITERAL_PATH("au")
        / PFS__LITERAL_PATH("stereol.wav");
    ionik::audio::wav_explorer wav_explorer { au_path };
    ionik::audio::wav_info wav_info;

    fmt::println("DECODE INFO: Test decoding of: {}", au_path);

    wav_explorer.on_error = [] (ionik::error const & err) {
        fmt::println(stderr, "DECODE ERROR: {}", err.what());
    };

    wav_explorer.on_wav_info = [& wav_info] (ionik::audio::wav_info const & winfo, std::size_t *) {
        wav_info = winfo;
        fmt::println("DECODE INFO: duration: {}"
            , ionik::audio::stringify_duration(wav_info.duration));

        if (!(wav_info.byte_order == pfs::endian::little
                && wav_info.byte_order == pfs::endian::native)) {
            MESSAGE("Only little endian platform and little byte"
                " order participates in tests yet");
            return false;
        }

        return true;
    };

    wav_explorer.on_raw_data = [& wav_info] (char const * raw_samples, std::size_t size) {
        if (wav_info.sample_size == 8 && wav_info.num_channels == 1) {
            fmt::println("DECODE INFO: 8 bits Mono, samples buffer size: {}", size);
        } else if (wav_info.sample_size == 8 && wav_info.num_channels == 2) {
            fmt::println("DECODE INFO: 8 bits Stereo, samples buffer size: {}", size);
        } else if (wav_info.sample_size == 16 && wav_info.num_channels == 1) {
            fmt::println("DECODE INFO: 16 bits Mono, samples buffer size: {}", size);
        } else if (wav_info.sample_size == 16 && wav_info.num_channels == 2) {
            // Test wav file meets this case

            auto samples = reinterpret_cast<std::int16_t const *>(raw_samples);
            auto samples_count = size / sizeof(std::int16_t);
            fmt::println("DECODE INFO: 16 bits Stereo, samples buffer size: {}"
                ", samples count: {}", size, samples_count);

            REQUIRE_EQ(size % samples_count, 0);

            using frame_iterator = ionik::audio::s16_stereo_frame_iterator;
            frame_iterator pos {raw_samples};
            frame_iterator last {raw_samples + size};

            std::size_t i = 0;
            frame_iterator::value_type frame;

            for (; pos != last; ++pos, i += 2) {
                frame = *pos;
                // fmt::println("Left: {:>6}, right: {:>6}", frame.left, frame.right);

                if (frame.left != samples[i]) {
                    MESSAGE("Iterator implementation is invalid, cneed to correct");
                    return false;
                }

                if (frame.right != samples[i + 1]) {
                    MESSAGE("Iterator implementation is invalid, cneed to correct");
                    return false;
                }
            }

            fmt::println("LAST FRAME: left: {:>6}, right: {:>6}", frame.left, frame.right);

            REQUIRE_EQ(i, samples_count);

        } else {
            MESSAGE(fmt::format("Sample size {} bits with {} channels does not"
                " participate in tests yet", wav_info.sample_size, wav_info.num_channels));
            return false;
        }

        return true;
    };

    CHECK(wav_explorer.decode(1024));
}
