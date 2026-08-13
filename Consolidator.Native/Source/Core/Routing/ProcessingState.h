#pragma once

#include <array>
#include <cstddef>

#include "Core/Domain/State/ChainState.h"

namespace consolidator::core
{

struct ResolvedProcessingState
{
    bool saturatorActive = false;
    bool compressorActive = false;
    bool equalizerActive = false;
    std::array<bool, InstanceState::kBankCount> equalizerBanksActive{};
    std::array<std::array<bool, 7>, InstanceState::kBankCount>
        equalizerFiltersActive{};
    std::array<bool, 2> saturatorDetectorFiltersActive{};
    std::array<bool, 2> compressorDetectorFiltersActive{};
};

inline ResolvedProcessingState ResolveProcessingState(
    const ChainState& chain) noexcept
{
    ResolvedProcessingState result;
    constexpr std::size_t noSolo = 5;
    std::size_t lastSolo = noSolo;
    if (chain.saturator.solo.value) lastSolo = 1;
    if (chain.compressor.solo.value) lastSolo = 2;
    if (chain.equalizer.solo.value) lastSolo = 3;
    const auto isAllowed = [lastSolo](std::size_t index)
    {
        return lastSolo == noSolo || index <= lastSolo;
    };

    result.saturatorActive = !chain.saturator.bypass.value && isAllowed(1);
    result.compressorActive = !chain.compressor.bypass.value && isAllowed(2);
    result.equalizerActive = !chain.equalizer.bypass.value && isAllowed(3);

    bool anyBankSolo = false;
    for (const auto& bank : chain.equalizers)
        anyBankSolo = anyBankSolo || bank.solo.value;

    for (std::size_t bankIndex = 0;
         bankIndex < result.equalizerBanksActive.size(); ++bankIndex)
    {
        const auto& bank = chain.equalizers[bankIndex];
        const bool bankActive = result.equalizerActive &&
            !bank.bypass.value && (!anyBankSolo || bank.solo.value);
        result.equalizerBanksActive[bankIndex] = bankActive;

        bool anyFilterSolo = false;
        for (const auto& filter : bank.filters)
            anyFilterSolo = anyFilterSolo || filter.solo.value;
        for (std::size_t filterIndex = 0;
             filterIndex < result.equalizerFiltersActive[bankIndex].size();
             ++filterIndex)
        {
            const auto& filter = bank.filters[filterIndex];
            result.equalizerFiltersActive[bankIndex][filterIndex] =
                bankActive && !filter.bypass.value &&
                (!anyFilterSolo || filter.solo.value);
        }
    }

    const auto resolveDetector = [](const auto& detector,
                                    bool parentActive,
                                    auto& active)
    {
        bool anyFilterSolo = false;
        for (const auto& filter : detector.filters)
            anyFilterSolo = anyFilterSolo || filter.solo.value;
        for (std::size_t index = 0; index < active.size(); ++index)
        {
            const auto& filter = detector.filters[index];
            active[index] = (parentActive || detector.listen.value) &&
                !filter.bypass.value &&
                (!anyFilterSolo || filter.solo.value);
        }
    };

    resolveDetector(chain.saturator.detector, result.saturatorActive,
                    result.saturatorDetectorFiltersActive);
    resolveDetector(chain.compressor.detector, result.compressorActive,
                    result.compressorDetectorFiltersActive);
    return result;
}

} // namespace consolidator::core
