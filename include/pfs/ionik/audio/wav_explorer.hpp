////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2023 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2023.10.10 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/ionik/error.hpp"
#include "pfs/ionik/exports.hpp"
#include "pfs/ionik/local_file.hpp"
#include "pfs/expected.hpp"
#include "pfs/endian.hpp"
#include "pfs/filesystem.hpp"
#include "pfs/iterator.hpp"
#include "pfs/expected.hpp"
#include <functional>
#include <limits>
#include <utility>

namespace ionik {
namespace audio {

struct wav_chunk_info {
    std::uint32_t id;
    std::uint32_t size;
    std::uint32_t start_offset; // offset of chunk data in the file
};

struct wav_info
{
    pfs::endian byte_order;
    int audio_format; // 1 -> PCM
    int num_channels; // Mono = 1, Stereo = 2, etc.
    std::uint32_t sample_rate;  // 8000, 44100, etc.
    int sample_size;            // Bits per sample: 8 bits = 8, 16 bits = 16, etc.
    std::uint32_t byte_rate;    // sample_rate * num_channels * sample_size / 8
    std::uint32_t sample_count; // Total count of samples
    std::uint32_t frame_count;  // Total count of frames
    std::uint64_t duration;     // Total duration in microseconds
    wav_chunk_info data;        // Data parameters
    std::vector<wav_chunk_info> extra; // Extra parameters
};

inline constexpr bool is_mono8 (wav_info const & info)
{
    return info.sample_size <= 8 && info.num_channels == 1;
}

inline constexpr bool is_stereo8 (wav_info const & info)
{
    return info.sample_size <= 8 && info.num_channels == 2;
}

inline constexpr bool is_mono16 (wav_info const & info)
{
    return info.sample_size <= 16 && info.num_channels == 1;
}

inline constexpr bool is_stereo16 (wav_info const & info)
{
    return info.sample_size <= 16 && info.num_channels == 2;
}

class wav_explorer
{
    local_file _wav_file;

public:
    // Decoder callbacks
    mutable std::function<void (error const &)> on_error = [] (error const &) {};

    /** Return @c false to interrupt decoding. Second argument is a pointer to
     * @c frames_chunk_size that passed to the @c decode function. It's value
     * can be corrected after reading of WAV header.
     */
    mutable std::function<bool (wav_info const &, std::size_t *)> on_wav_info
        = [] (wav_info const &, std::size_t *) {return true;};

    /// Return @c false to interrupt decoding.
    mutable std::function<bool (char const *, std::size_t)> on_raw_data
        = [] (char const *, std::size_t) {return true;};

public:
    IONIK__EXPORT wav_explorer (local_file && wav_file);
    IONIK__EXPORT wav_explorer (pfs::filesystem::path const & path, error * perr = nullptr);

    IONIK__EXPORT pfs::expected<wav_info, error> read_header ();
    IONIK__EXPORT bool decode (std::size_t frames_chunk_size = 1024);
};

template <typename SampleType>
struct mono_frame
{
    static const constexpr int sizeof_frame = sizeof(SampleType);

    SampleType sample;

    explicit mono_frame (SampleType value = 0): sample(value) {}

    explicit mono_frame (char const * p)
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        : sample(*reinterpret_cast<SampleType const *>(p))
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        : sample(pfs::byteswap(*reinterpret_cast<SampleType const *>(p)))
#endif
    {}
};

template <typename SampleType>
struct stereo_frame
{
    static const constexpr int sizeof_frame = sizeof(SampleType) * 2;

    SampleType left;
    SampleType right;

    explicit stereo_frame (SampleType l = 0, SampleType r = 0)
        : left(l), right(r)
    {}

    explicit stereo_frame (char const * p)
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        : left(*reinterpret_cast<SampleType const *>(p))
            , right(*reinterpret_cast<SampleType const *>(p + sizeof(SampleType)))
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        : left(pfs::byteswap(*reinterpret_cast<SampleType const *>(p)))
        , right(pfs::byteswap(*reinterpret_cast<SampleType const *>(p + sizeof(SampleType))))
#endif
    {}
};

template <typename FrameType>
class frame_iterator : public pfs::iterator_facade<
      pfs::random_access_iterator_tag
    , frame_iterator<FrameType>
    , FrameType, char const *, FrameType>
{
public:
    using difference_type = typename frame_iterator<FrameType>::difference_type;

private:
    char const * _p {nullptr};

public:
    frame_iterator (char const * p)
        : _p(p)
    {}

    typename frame_iterator<FrameType>::reference ref ()
    {
        return FrameType{_p};
    }

    typename frame_iterator<FrameType>::pointer ptr ()
    {
        return _p;
    }

    void increment (typename frame_iterator<FrameType>::difference_type n)
    {
        _p += n * FrameType::sizeof_frame;
    }

    int compare (frame_iterator<FrameType> const & rhs) const
    {
        return _p - rhs._p;
    }

    void decrement (typename frame_iterator<FrameType>::difference_type n)
    {
        _p -= n * FrameType::sizeof_frame;
    }

    typename frame_iterator<FrameType>::reference subscript (typename frame_iterator<FrameType>::difference_type n)
    {
        return FrameType{_p + n * FrameType::sizeof_frame};
    }

    typename frame_iterator<FrameType>::difference_type diff (frame_iterator<FrameType> const & rhs) const
    {
        return (_p - rhs._p) / FrameType::sizeof_frame;
    }
};

using u8_mono_frame_iterator    = frame_iterator<mono_frame<std::uint8_t>>;
using s8_mono_frame_iterator    = frame_iterator<mono_frame<std::int8_t>>;
using u8_stereo_frame_iterator  = frame_iterator<stereo_frame<std::uint8_t>>;
using s8_stereo_frame_iterator  = frame_iterator<stereo_frame<std::int8_t>>;
using u16_mono_frame_iterator   = frame_iterator<mono_frame<std::uint16_t>>;
using s16_mono_frame_iterator   = frame_iterator<mono_frame<std::int16_t>>;
using u16_stereo_frame_iterator = frame_iterator<stereo_frame<std::uint16_t>>;
using s16_stereo_frame_iterator = frame_iterator<stereo_frame<std::int16_t>>;
using f32_mono_frame_iterator   = frame_iterator<mono_frame<float>>;
using f32_stereo_frame_iterator = frame_iterator<stereo_frame<float>>;

struct wav_spectrum
{
    // For mono frames second part of pair is unused
    using unified_frame = std::pair<float, float>;

    unified_frame min_frame;
    unified_frame max_frame;
    std::vector<unified_frame> data;
    wav_info info;
};

class wav_spectrum_builder
{
    struct builder_context
    {
        std::size_t frame_step;
        error err;
        wav_spectrum spectrum;
    };

private:
    wav_explorer * _explorer {nullptr};

    bool (wav_spectrum_builder::*_build_proc) (builder_context &, char const *, std::size_t) {nullptr};

private:
    bool build_from_mono8 (builder_context &, char const *, std::size_t);
    bool build_from_stereo8 (builder_context & ctx, char const *, std::size_t);
    bool build_from_mono16 (builder_context & ctx, char const *, std::size_t);
    bool build_from_stereo16 (builder_context & ctx, char const *, std::size_t);

public:
    wav_spectrum_builder (wav_explorer & explorer)
        : _explorer(& explorer)
    {}

    pfs::expected<wav_spectrum, error> operator () (std::size_t chunk_count
        , std::size_t frame_step = (std::numeric_limits<std::size_t>::max)());
};

enum class duration_precision
{
      microseconds
    , milliseconds
    , seconds
    , minutes
    , hours
};

IONIK__EXPORT std::string stringify_duration (std::uint64_t microseconds
    , duration_precision prec = duration_precision::seconds);

}} // namespace ionik::audio
