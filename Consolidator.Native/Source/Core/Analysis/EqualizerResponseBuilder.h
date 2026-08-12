#pragma once

#include "Analysis/FrequencyResponse/FrequencyResponseTypes.h"
#include "Core/Domain/State/StateStore.h"

namespace consolidator::core
{

class EqualizerResponseBuilder final
{
public:
    [[nodiscard]] analysis::FrequencyResponseRequest Build(
        const StateStore& stateStore,
        double sampleRate,
        std::uint64_t revision) const noexcept;
};

} // namespace consolidator::core
