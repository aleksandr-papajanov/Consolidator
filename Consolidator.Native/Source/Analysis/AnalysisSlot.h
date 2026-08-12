#pragma once

#include "Analysis/FrequencyResponse/FrequencyResponseStream.h"
#include "Analysis/Spectrum/SpectrumStream.h"

namespace consolidator::analysis
{

// Owns all analysis streams belonging to one instance.
class AnalysisSlot final
{
public:
    AnalysisSlot();

    [[nodiscard]] SpectrumStream& MainSpectrum() noexcept
    {
        return mainSpectrum_;
    }

    [[nodiscard]] SpectrumStream& ReferenceSpectrum() noexcept
    {
        return referenceSpectrum_;
    }

    [[nodiscard]] FrequencyResponseStream& EqualizerResponse() noexcept
    {
        return equalizerResponse_;
    }

private:
    SpectrumStream mainSpectrum_;
    SpectrumStream referenceSpectrum_;
    FrequencyResponseStream equalizerResponse_;
};

} // namespace consolidator::analysis
