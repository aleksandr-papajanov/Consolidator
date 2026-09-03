#include "DspRuntime.h"

namespace consolidator::max::dsp
{

void DspRuntime::Prepare(
    double sampleRate,
    std::size_t maximumFrameCount)
{
    graph_.Prepare(sampleRate, maximumFrameCount);
}

void DspRuntime::Reset() noexcept
{
    graph_.Reset();
}

void DspRuntime::ProcessBlock(
    DspStateExchange& exchange,
    double* mainLeft,
    double* mainRight,
    std::size_t frameCount) noexcept
{
    if (ConsumePublishedDspState(
            exchange,
            consumerIndex_,
            snapshot_))
    {
        graph_.SetParameters(snapshot_);
    }

    graph_.ProcessBlock(
        mainLeft,
        mainRight,
        frameCount);
}

} // namespace consolidator::max::dsp
