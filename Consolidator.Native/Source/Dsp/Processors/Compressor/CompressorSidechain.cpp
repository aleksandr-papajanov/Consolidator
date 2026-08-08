#include "Dsp/Processors/Compressor/CompressorSidechain.h"

#include <memory>

#include "Dsp/Processors/Equalizer/Filters/BellFilter.h"
#include "Dsp/Processors/Equalizer/Filters/LowShelfFilter.h"

namespace consolidator::dsp
{

CompressorSidechain::CompressorSidechain(
    CompressorDetectorFilterId lowShelfId,
    CompressorDetectorFilterId bellId)
{
    filters_.AddFilter(std::make_unique<LowShelfFilter>(
        detail::ToFilterId(detail::ToIndex(lowShelfId)),
        100.0));

    filters_.AddFilter(std::make_unique<BellFilter>(
        detail::ToFilterId(detail::ToIndex(bellId)),
        1000.0));
}

void CompressorSidechain::Prepare(double sampleRate)
{
    filters_.Prepare(sampleRate, 1);
}

void CompressorSidechain::Reset() noexcept
{
    filters_.Reset();
}

double CompressorSidechain::ProcessSample(double input) noexcept
{
    return filters_.ProcessSample(input);
}

bool CompressorSidechain::ApplyParameter(
    const ParameterRoute& route,
    const ParameterValue& value,
    std::size_t depth)
{
    return filters_.ApplyParameter(route, value, depth);
}

} // namespace consolidator::dsp
