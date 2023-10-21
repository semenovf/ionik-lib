////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2023 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2023.10.10 Initial version.
//
// Sources:
//      1. https://ru.stackoverflow.com/questions/878097/Как-нарисовать-графическое-представление-wav-файла
//      2. https://ru.stackoverflow.com/questions/760083/wav-data-как-сидят-данные
//      3. http://netghost.narod.ru/gff/graphics/summary/micriff.htm
//      4. http://www.topherlee.com/software/pcm-tut-wavformat.html
//      5. [WAVE PCM soundfile format](https://web.archive.org/web/20140327141505/https://ccrma.stanford.edu/courses/422/projects/WaveFormat/)
//      6. [Audio File Format Specifications](https://www-mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/WAVE.html)
//      7. [WAVE Sample Files](https://www.mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/Samples.html)
//
////////////////////////////////////////////////////////////////////////////////
#include "pfs/ionik/audio/wav_explorer.hpp"
#include "pfs/binary_istream.hpp"
#include "pfs/endian.hpp"
#include "pfs/numeric_cast.hpp"
#include "pfs/string_view.hpp"
#include "pfs/ionik/local_file.hpp"
#include <cstdint>

#include "pfs/log.hpp"

namespace ionik {
namespace audio {

// RIFF Chunk Descriptor
struct wav_header
{
    // The canonical WAVE format starts with the RIFF header:
    std::uint32_t chunk_id;        // Contains the letters "RIFF" in ASCII form (0x52494646 big-endian form) or RIFX if big-endian byte ordering scheme is assumed
    std::uint32_t chunk_size;      // 4 + (8 + subchunk1_size) + (8 + subchunk2_size)
    std::uint32_t format;          // Contains the letters "WAVE" (0x57415645 big-endian form)

    // The "WAVE" format consists of two subchunks: "fmt " and "data":
    // The "fmt " subchunk describes the sound data's format:
    std::uint32_t subchunk1_id;    // Contains the letters "fmt " (0x666d7420 big-endian form).
    std::uint32_t subchunk1_size;  // Size of the fmt chunk, 16 for PCM.
    std::uint16_t audio_format;    // Audio format 1=PCM, 6=mulaw, 7=alaw, 257=IBM Mu-Law, 258=IBM A-Law, 259=ADPCM
    std::uint16_t num_channels;    // Number of channels: Mono = 1, Stereo = 2, etc.
    std::uint32_t sample_rate;     // Sampling Frequency in Hz: 8000, 44100, etc.
    std::uint32_t byte_rate;       // Bytes per second (byteRate): sample_rate * num_channels * sample_size/8
    std::uint16_t block_align;     // The number of bytes for one sample including all channels: num_channels * sample_size / 8: 2=16-bit mono, 4=16-bit stereo
    std::uint16_t sample_size;     // Number of bits per sample: 8 bits = 8, 16 bits = 16, etc.

//     // // The "data" subchunk contains the size of the data and the actual sound:
//     // std::uint32_t subchunk2_id;    // Contains the letters "data" (0x64617461 big-endian form).
//     // std::uint32_t subchunk2_size;  // The number of bytes in the data: NumSamples * num_channels * sample_size/8
};

// The data format and maximum and minimums values for PCM waveform samples of
// various sizes are as follows:
//
// Sample Size |   Data Format   | Maximum Value  |  Minimum Value
// ------------|-----------------|----------------|-----------------
//   1- 8 bits |   std::uint8_t  |           255  |               0
//   9-16 bits |   std::int16_t  | 32767 (0x7FFF) | -32768 (-0x8000)

static constexpr const local_file::filesize_type WAV_HEADER_SIZE
    = 7 * sizeof(std::uint32_t) + 4 * sizeof(std::uint16_t);

// "fmt " chunk common size (really can be greater)
static constexpr const local_file::filesize_type WAV_SUBCHUNK1_SIZE
    = 2 * sizeof(std::uint32_t) + 4 * sizeof(std::uint16_t);

wav_explorer::wav_explorer (local_file && wav_file)
    : _wav_file(std::move(wav_file))
{}

wav_explorer::wav_explorer (pfs::filesystem::path const & path, error * perr)
{
    if (!pfs::filesystem::exists(path)) {
        pfs::throw_or(perr
            , std::make_error_code(std::errc::no_such_file_or_directory)
            , pfs::filesystem::utf8_encode(path));

        return;
    }

    _wav_file = local_file::open_read_only(path, perr);

    if (!_wav_file)
        return;
}

pfs::expected<wav_info, error> wav_explorer::read_header ()
{
    wav_info info;
    char buffer[WAV_HEADER_SIZE];

    error err;
    auto res = _wav_file.read(buffer, WAV_HEADER_SIZE, & err);

    if (!res.second)
        return pfs::make_unexpected(err);

    if (res.first < WAV_HEADER_SIZE)
        return pfs::make_unexpected(error {errc::bad_data_format});

    pfs::binary_istream<pfs::endian::little> is {buffer
        , WAV_HEADER_SIZE};

    wav_header header;

    is >> header.chunk_id
        >> header.chunk_size
        >> header.format
        >> header.subchunk1_id
        >> header.subchunk1_size
        >> header.audio_format
        >> header.num_channels
        >> header.sample_rate
        >> header.byte_rate
        >> header.block_align
        >> header.sample_size;

    pfs::string_view chunk_id{reinterpret_cast<char const *>(& header.chunk_id)
         , sizeof(header.chunk_id)};

    if (chunk_id == "RIFF") {
        info.byte_order = pfs::endian::little;
    } else if (chunk_id == "RIFX") {
        info.byte_order = pfs::endian::big;
    } else {
        return pfs::make_unexpected(error {errc::unsupported, tr::_("file format")});
    }

    pfs::string_view format{reinterpret_cast<char const *>(& header.format)
        , sizeof(header.format)};

    if (format != "WAVE")
        return pfs::make_unexpected(error {errc::unsupported, tr::_("file format")});

    pfs::string_view subchunk1_id{reinterpret_cast<char const *>(& header.subchunk1_id)
        , sizeof(header.subchunk1_id)};

    if (subchunk1_id != "fmt ")
        return pfs::make_unexpected(error {errc::unsupported, tr::_("file format")});

    switch (header.audio_format) {
        case 1:   // PCM - Pulse Code Modulation Format (codec=audio/pcm)
        case 6:   // mulaw
        case 7:   // alaw
        case 257: // IBM Mu-Law
        case 258: // IBM A-Law
        case 259: // ADPCM
            info.audio_format = header.audio_format;
            break;
        default:
            return pfs::make_unexpected(error {errc::unsupported
                , tr::f_("audio format: {}", header.audio_format)});
    }

    switch (header.num_channels) {
        case 1: // Mono
        case 2: // Stereo
            info.num_channels = header.num_channels;
            break;
        default:
            return pfs::make_unexpected(error { errc::unsupported
                , tr::f_("number of channels: {}", header.num_channels)});
    }

    // Skip data if common "fmt " subchunk is less than count specified in subchunk1_size
    if (header.subchunk1_size > WAV_SUBCHUNK1_SIZE) {
        if (!_wav_file.skip(header.subchunk1_size - WAV_SUBCHUNK1_SIZE, & err))
            return pfs::make_unexpected(err);
    }

    std::uint32_t subchunk2_id;
    std::uint32_t subchunk2_size;

    // Read extra parameters till data section
    do {
        auto chunk_header_size = 2 * sizeof(std::uint32_t);

        // `buffer` already not need here, it can be reused.
        res = _wav_file.read(buffer, chunk_header_size, & err);

        if (!res.second)
            return pfs::make_unexpected(err);

        if (res.first < chunk_header_size)
            return pfs::make_unexpected(error {errc::bad_data_format});

        pfs::binary_istream<pfs::endian::little> is {buffer, chunk_header_size};
        is >> subchunk2_id >> subchunk2_size;

        if (pfs::endian::native == pfs::endian::little)
            subchunk2_id = pfs::byteswap(subchunk2_id);

        // The letters "data" (0x64617461 big-endian form).
        if (subchunk2_id == 0x64617461)
            break;

        auto off_res = _wav_file.offset(& err);

        if (!off_res.second)
            return pfs::make_unexpected(err);

        if (!_wav_file.skip(subchunk2_size, & err))
            return pfs::make_unexpected(err);

        info.extra.emplace_back(wav_chunk_info {
              subchunk2_id
            , subchunk2_size
            , pfs::numeric_cast<std::uint32_t>(off_res.first)
        });
    } while(true);

    // The "data" subchunk contains the size of the data and the actual sound:
    // the number of bytes in the data: NumSamples * num_channels * sample_size/8
    if (subchunk2_id != 0x64617461) // "data"
        return pfs::make_unexpected(error {errc::unsupported, tr::_("file format")});

    // "data" subchunk
    {
        auto off_res = _wav_file.offset(& err);

        if (!off_res.second)
            return pfs::make_unexpected(err);

        info.data.id = subchunk2_id;
        info.data.size = subchunk2_size;
        info.data.start_offset = pfs::numeric_cast<std::uint32_t>(off_res.first);
    }

    info.byte_rate    = header.byte_rate;
    info.sample_rate  = header.sample_rate;
    info.sample_size  = pfs::numeric_cast<decltype(wav_info::sample_size)>(header.sample_size);
    info.sample_count = subchunk2_size / (header.sample_size / 8);
    info.frame_count  = subchunk2_size / header.block_align;
    info.duration     = static_cast<float>(subchunk2_size) / header.byte_rate
        * std::uint64_t{1000} * std::uint64_t{1000};

    return info;
}

bool wav_explorer::decode (std::size_t frames_chunk_size)
{
    auto hdr = read_header();

    if (!hdr) {
        on_error(hdr.error());
        return false;
    }

    if (hdr->audio_format != 1) { // PCM
        on_error(error {errc::unsupported, tr::_("only PCM format supported for decoding")});
        return false;
    }

    if (hdr->sample_size > 16) {
        on_error(error {errc::unsupported, tr::_("sample size: {} bits (only size <= 16 bits supported now)")});
        return false;
    }

    error err;

    // Interrupted
    if (!on_wav_info(*hdr, & frames_chunk_size))
        return false;

    std::vector<char> raw_buffer;
    std::size_t raw_buffer_size = frames_chunk_size * hdr->num_channels;
    std::size_t remain_size = hdr->data.size;

    if (hdr->sample_size <= 8)
        ;
    else if (hdr->sample_size <= 16)
        raw_buffer_size *= 2;
    else if (hdr->sample_size <= 32)
        raw_buffer_size *= 4;

    raw_buffer.resize(raw_buffer_size);

    // File offset in the begining of samples data now.

    do {
        auto res = remain_size > raw_buffer.size()
            ? _wav_file.read(raw_buffer.data(), raw_buffer.size(), & err)
            : _wav_file.read(raw_buffer.data(), remain_size, & err);

        // Read failure
        if (!res.second)
            break;

        if (res.first > 0) {
            // Interrupted
            if (!on_raw_data(raw_buffer.data(), res.first))
                return false;
        } else {
            break;
        }

        remain_size -= res.first;
    } while (remain_size > 0);

    if (err) {
        on_error(err);
        return false;
    }

    return true;
}

pfs::expected<wav_spectrum, error>
wav_spectrum_builder::operator () (std::size_t chunk_count
    , std::size_t frame_step)
{
    if (chunk_count == 0) {
        return pfs::unexpected<error>(error{
              std::make_error_code(std::errc::invalid_argument)
            , tr::_("chunk count must be greater than 0")
        });
    }

    builder_context ctx;
    ctx.frame_step = frame_step;

    _explorer->on_error = [& ctx] (error const & e) { ctx.err = e; };

    _explorer->on_wav_info = [this, & ctx, chunk_count] (ionik::audio::wav_info const & info
            , std::size_t * frames_chunk_size) {
        ctx.spectrum.max_frame = std::make_pair(-1.0f, -1.0f);
        ctx.spectrum.min_frame = std::make_pair( 1.0f,  1.0f);
        ctx.spectrum.info = info;

        if (ctx.spectrum.info.sample_size > 16) {
            ctx.err = error {errc::unsupported, tr::_("sample size greater than 16")};
            return false;
        }

        if (is_mono8(ctx.spectrum.info)) {
            _build_proc = & wav_spectrum_builder::build_from_mono8;
        } else if (is_stereo8(ctx.spectrum.info)) {
            _build_proc = & wav_spectrum_builder::build_from_stereo8;
        } else if (is_mono16(ctx.spectrum.info)) {
            _build_proc = & wav_spectrum_builder::build_from_mono16;
        } else if (is_stereo16(ctx.spectrum.info)) {
            _build_proc = & wav_spectrum_builder::build_from_stereo16;
        } else {
            ctx.err = error {errc::unsupported, tr::f_("8/16 bits and mono/stereo only")};
            return false;
        }

        *frames_chunk_size = ctx.spectrum.info.frame_count / chunk_count;
        auto tail_size = ctx.spectrum.info.frame_count % chunk_count;

        if (tail_size != 0) {
            *frames_chunk_size = (ctx.spectrum.info.frame_count - tail_size)
                / (chunk_count - 1);
        }

        return true;
    };

    _explorer->on_raw_data = [this, & ctx] (char const * raw_samples, std::size_t size) {
        return (this->*_build_proc)(ctx, raw_samples, size);
    };

    if (!_explorer->decode())
        return pfs::unexpected<error>(ctx.err);

    return ctx.spectrum;
}

inline float normalize_sample8 (std::uint8_t value)
{
    float res = (value - 128.0f) / 255.0f;

    if (res > 1.0f)
        res = 1.0f;

    if (res < -1.0f)
        res = -1.0f;

    return res;
}

inline float normalize_sample16 (std::int16_t value)
{
    float res = value / 32767.0f;

    if (res > 1.0f)
        res = 1.0f;

    if (res < -1.0f)
        res = -1.0f;

    return res;
}

bool wav_spectrum_builder::build_from_mono8 (builder_context & ctx
    , char const * raw_samples, std::size_t size)
{
    // TODO Implement
    ctx.err = error {errc::unsupported, tr::_("not implemented yet: mono8")};
    return false;
}

bool wav_spectrum_builder::build_from_stereo8 (builder_context & ctx
    , char const * raw_samples, std::size_t size)
{
    // HERE-----------------------------------v
    auto samples_count = size / sizeof(std::uint8_t);

    if (size % samples_count != 0) {
        ctx.err = error {errc::bad_data_format, tr::_("bad data format or data may be corrupted")};
        return false;
    }

    // HERE------------------------------------------v
    using frame_iterator = ionik::audio::u8_stereo_frame_iterator;
    frame_iterator pos {raw_samples};
    frame_iterator last {raw_samples + size};
    frame_iterator::value_type frame;
    std::size_t count = 0;
    float left_sum = 0;
    float right_sum = 0;

    for (; pos < last; pos += ctx.frame_step) {
        frame = *pos;

        // HERE----------------v
        float left  = normalize_sample8(frame.left);
        float right = normalize_sample8(frame.right);

        left_sum += left;
        right_sum += right;
        count++;
    }

    if (count > 0) {
        float left  = left_sum / count;
        float right = right_sum / count;

        if (left > ctx.spectrum.max_frame.first)
            ctx.spectrum.max_frame.first = left;

        if (right > ctx.spectrum.max_frame.second)
            ctx.spectrum.max_frame.second = right;

        if (left < ctx.spectrum.min_frame.first)
            ctx.spectrum.min_frame.first = left;

        if (right < ctx.spectrum.min_frame.second)
            ctx.spectrum.min_frame.second = right;

        ctx.spectrum.data.push_back(std::make_pair(left, right));
    } else {
        ctx.spectrum.data.push_back(std::make_pair(0.f, 0.f));
    }

    return true;
}

bool wav_spectrum_builder::build_from_mono16 (builder_context & ctx
    , char const * raw_samples, std::size_t size)
{
    ctx.err = error {errc::unsupported, tr::_("not implemented yet")};
    return false;
}

bool wav_spectrum_builder::build_from_stereo16 (builder_context & ctx
    , char const * raw_samples, std::size_t size)
{
    auto samples_count = size / sizeof(std::int16_t);

    if (size % samples_count != 0) {
        ctx.err = error {errc::bad_data_format, tr::_("bad data format or data may be corrupted")};
        return false;
    }
    using frame_iterator = ionik::audio::s16_stereo_frame_iterator;
    frame_iterator pos {raw_samples};
    frame_iterator last {raw_samples + size};
    frame_iterator::value_type frame;
    std::size_t count = 0;
    float left_sum = 0;
    float right_sum = 0;

    for (; pos < last; pos += ctx.frame_step) {
        frame = *pos;
        float left  = normalize_sample16(frame.left);
        float right = normalize_sample16(frame.right);

        left_sum += left;
        right_sum += right;
        count++;
    }

    if (count > 0) {
        float left  = left_sum / count;
        float right = right_sum / count;

        if (left > ctx.spectrum.max_frame.first)
            ctx.spectrum.max_frame.first = left;

        if (right > ctx.spectrum.max_frame.second)
            ctx.spectrum.max_frame.second = right;

        if (left < ctx.spectrum.min_frame.first)
            ctx.spectrum.min_frame.first = left;

        if (right < ctx.spectrum.min_frame.second)
            ctx.spectrum.min_frame.second = right;

        ctx.spectrum.data.push_back(std::make_pair(left, right));
    } else {
        ctx.spectrum.data.push_back(std::make_pair(0.f, 0.f));
    }

    return true;
}

std::string stringify_duration (std::uint64_t microseconds, duration_precision prec)
{
    auto micros  = microseconds % (1000 * 1000);
    auto millis  = (microseconds / 1000) % 1000;
    auto seconds = (((microseconds / 1000) - millis) / 1000) % 60;
    auto minutes = (((((microseconds / 1000) - millis) / 1000) - seconds) / 60) % 60;
    auto hours   = ((((((microseconds / 1000) - millis) / 1000) - seconds) / 60) - minutes) / 60;

    if (prec == duration_precision::seconds)
        return fmt::format("{}:{:02}:{:02}", hours, minutes, seconds);

    if (prec == duration_precision::milliseconds)
        return fmt::format("{}:{:02}:{:02}.{:03}", hours, minutes, seconds, millis);

    if (prec == duration_precision::microseconds)
        return fmt::format("{}:{:02}:{:02}.{:06}", hours, minutes, seconds, micros);

    if (prec == duration_precision::minutes)
        return fmt::format("{}:{:02}", hours, minutes);

    if (prec == duration_precision::hours)
        return fmt::format("{}", hours);

    // Same as duration_precision::seconds
    return fmt::format("{}:{:02}:{:02}", hours, minutes, seconds);
}

}} // namespace ionik::audio
