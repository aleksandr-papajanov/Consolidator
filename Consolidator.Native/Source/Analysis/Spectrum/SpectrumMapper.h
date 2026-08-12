#pragma once

#include "Analysis/Spectrum/SpectrumTypes.h"

namespace consolidator::analysis
{

// Maps raw linear FFT magnitudes to a logarithmic display curve in dB.
class SpectrumMapper final
{
public:
    void Calculate(
        const RawSpectrum& input,
        SpectrumSnapshot& output) const noexcept;
};

} // namespace consolidator::analysis
