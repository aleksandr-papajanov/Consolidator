#pragma once

#include "Dsp/Parameters/ParameterAddress.h"
#include "Dsp/Parameters/ParameterChange.h"
#include "Dsp/Processors/Equalizer/Filters/BellFilter.h"
#include "Dsp/Processors/Equalizer/Filters/LowShelfFilter.h"

namespace consolidator::dsp
{

class CompressorSidechain
{
public:
    CompressorSidechain(
        CompressorDetectorFilterId lowShelfId,
        CompressorDetectorFilterId bellId);

    void Prepare(double sampleRate);
    void Reset() noexcept;

    [[nodiscard]] double ProcessSample(double input) noexcept;

    void ApplyParameterChange(const ParameterChange& change);

private:
    LowShelfFilter lowShelf_;
    BellFilter bell_;
};

} // namespace consolidator::dsp