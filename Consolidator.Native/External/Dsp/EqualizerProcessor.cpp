#include "DspProcessors.h"

#include <algorithm>

namespace consolidator::max::dsp
{

void EqualizerProcessor::Prepare(
    const DspProcessingContext& context) noexcept
{
    (void)context;
}

void EqualizerProcessor::Reset() noexcept
{
}

void EqualizerProcessor::SetParameters(
    const DspSnapshot& snapshot) noexcept
{
    std::copy_n(
        snapshot.equalizerBanksActive,
        activeBanks_.size(),
        activeBanks_.begin());
    std::copy_n(
        snapshot.equalizerFilters,
        filters_.size(),
        filters_.begin());
}

void EqualizerProcessor::Process(
    DspProcessingFrame& frame) noexcept
{
    (void)frame;
}

} // namespace consolidator::max::dsp
