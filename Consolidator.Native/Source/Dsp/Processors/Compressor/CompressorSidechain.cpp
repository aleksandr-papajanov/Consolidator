#include "Dsp/Processors/Compressor/CompressorSidechain.h"

namespace consolidator::dsp
{

CompressorSidechain::CompressorSidechain(
    CompressorDetectorFilterId lowShelfId,
    CompressorDetectorFilterId bellId)
    : lowShelf_(detail::ToEqFilterId(detail::ToIndex(lowShelfId)), 100.0)
    , bell_(detail::ToEqFilterId(detail::ToIndex(bellId)), 1000.0)
{
}

void CompressorSidechain::Prepare(double sampleRate)
{
    lowShelf_.Prepare(sampleRate, 1);
    bell_.Prepare(sampleRate, 1);
    Reset();
}

void CompressorSidechain::Reset() noexcept
{
    lowShelf_.Reset();
    bell_.Reset();
}

double CompressorSidechain::ProcessSample(double input) noexcept
{
    const double lowShelfOutput = lowShelf_.ProcessSample(input, 0);
    return bell_.ProcessSample(lowShelfOutput, 0);
}

void CompressorSidechain::ApplyParameterChange(
    const ParameterChange& change)
{
    const auto& addr = change.address;

    if (addr.GetElementKind() == detail::ElementKind::CompressorDetectorFilter)
    {
        if (addr.GetElementIndex() == 0)
        {
            lowShelf_.ApplyParameterChange(change);
        }
        else
        {
            bell_.ApplyParameterChange(change);
        }
    }
}

} // namespace consolidator::dsp