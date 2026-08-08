#pragma once

#include "Dsp/Processors/Equalizer/Equalizer.h"

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


    bool ApplyParameter(
        const ParameterRoute& route,
        const ParameterValue& value,
        std::size_t depth);

private:
    Equalizer filters_{detail::ElementKind::CompressorDetectorFilter};
};

} // namespace consolidator::dsp
