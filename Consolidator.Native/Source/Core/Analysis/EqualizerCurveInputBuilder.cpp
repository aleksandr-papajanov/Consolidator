#include "Core/Analysis/EqualizerCurveInputBuilder.h"

#include <cstddef>

namespace consolidator::core
{

analysis::CurveInput EqualizerCurveInputBuilder::Build(
    const ChainState& chain,
    double sampleRate,
    std::uint64_t revision) const noexcept
{
    analysis::CurveInput input;
    input.sampleRate = sampleRate;
    input.revision = revision;
    input.equalizerActive = !chain.equalizer.bypass.value;

    std::size_t lastSoloChainIndex = 5;
    if (chain.saturator.solo.value)
    {
        lastSoloChainIndex = 1;
    }
    if (chain.compressor.solo.value)
    {
        lastSoloChainIndex = 2;
    }
    if (chain.equalizer.solo.value)
    {
        lastSoloChainIndex = 3;
    }
    input.chainAllowsEqualizer = lastSoloChainIndex >= 3;

    for (std::size_t bankIndex = 0; bankIndex < input.banks.size(); ++bankIndex)
    {
        const auto& sourceBank = chain.equalizers[bankIndex];
        auto& targetBank = input.banks[bankIndex];
        targetBank.bypass = sourceBank.bypass.value;
        targetBank.solo = sourceBank.solo.value;
        for (std::size_t filterIndex = 0;
             filterIndex < targetBank.filters.size();
             ++filterIndex)
        {
            const auto& source = sourceBank.filters[filterIndex];
            auto& target = targetBank.filters[filterIndex];
            target.frequencyHz = source.frequencyHz.value;
            target.q = source.q.value;
            target.gainDb = source.gainDb.value;
            target.bypass = source.bypass.value;
            target.solo = source.solo.value;
        }
    }
    return input;
}

} // namespace consolidator::core
