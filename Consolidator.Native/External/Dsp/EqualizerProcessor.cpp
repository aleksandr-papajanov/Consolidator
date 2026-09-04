#include "DspProcessors.h"

#include <algorithm>

namespace consolidator::max::dsp
{

void EqualizerProcessor::Prepare(
    const DspProcessingContext& context) noexcept
{
    for (auto& filter : filters_)
    {
        filter.Prepare(context);
    }
}

void EqualizerProcessor::Reset() noexcept
{
    for (auto& filter : filters_)
    {
        filter.Reset();
    }
}

void EqualizerProcessor::SetParameters(
    const DspSnapshot& snapshot) noexcept
{
    for (std::size_t bank = 0; bank < kBankCount; ++bank)
    {
        const auto bankEnabled = snapshot.equalizerActive != 0 &&
            snapshot.equalizerBypass == 0 &&
            snapshot.equalizerBanksActive[bank] != 0;
        for (std::size_t filter = 0; filter < kFilterCount; ++filter)
        {
            const auto index = bank * kFilterCount + filter;
            filters_[index].SetParameters(
                snapshot.equalizerFilters[index],
                bankEnabled);
        }
    }
}

void EqualizerProcessor::Process(
    DspProcessingFrame& frame) noexcept
{
    for (auto& filter : filters_)
    {
        filter.Process(frame.main, frame.plan.channelMode);
    }
}

bool EqualizerProcessor::RequiresProcessing() const noexcept
{
    return std::any_of(
        filters_.begin(),
        filters_.end(),
        [](const DspBiquadCascade& filter)
        {
            return filter.RequiresProcessing();
        });
}

} // namespace consolidator::max::dsp
