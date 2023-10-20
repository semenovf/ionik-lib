////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2023 Vladislav Trifochkin
//
// This file is part of `ionik-lib`.
//
// Changelog:
//      2023.10.20 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "WavSpectrum.hpp"
#include "pfs/numeric_cast.hpp"
#include <cmath>

WavSpectrum::WavSpectrum (QObject * parent)
    : QObject(parent)
{}

bool WavSpectrum::isStereo () const noexcept
{
    return _wavSpectrum.info.num_channels == 2;
}

int WavSpectrum::frameCount () const noexcept
{
    return pfs::numeric_cast<int>(_wavSpectrum.data.size());
}

void WavSpectrum::setSpectrum (ionik::audio::wav_spectrum && wavSpectrum)
{
    _good = true;
    _wavSpectrum = std::move(wavSpectrum);
    Q_EMIT(spectrumChanged());
}

void WavSpectrum::setSpectrum (ionik::audio::wav_spectrum const & wavSpectrum)
{
    _good = true;
    _wavSpectrum = wavSpectrum;
    Q_EMIT(spectrumChanged());
}

void WavSpectrum::setFailure ()
{
    _good = true;
    Q_EMIT(spectrumChanged());
}

float WavSpectrum::leftAt (int index) const
{
    if (index < 0 || index > _wavSpectrum.data.size())
        return 0.f;

    return _wavSpectrum.data.at(index).first;
}

float WavSpectrum::rightAt (int index) const
{
    if (index < 0 || index > _wavSpectrum.data.size())
        return 0.f;

    return _wavSpectrum.data.at(index).second;
}

float WavSpectrum::sampleAt (int index) const
{
    return leftAt(index);
}

float WavSpectrum::maxSampleValue () const
{
    float v = (std::max)((std::abs)(_wavSpectrum.max_frame.first)
        , (std::abs)(_wavSpectrum.min_frame.first));

    if (isStereo()) {
        float v1 = (std::max)((std::abs)(_wavSpectrum.max_frame.second)
            , (std::abs)(_wavSpectrum.min_frame.second));
        v = (std::max)(v, v1);
    }

    return v;
}
