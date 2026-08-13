#pragma once

#include "Analysis/CurveInput.h"
#include "Core/Domain/State/StateStore.h"

namespace consolidator::core
{

// Converts authoritative chain state into the immutable analysis input model.
class AnalysisCurveInputBuilder final
{
  public:
    [[nodiscard]] analysis::CurveInput Build(
        const ChainState& chain,
        double sampleRate,
        std::uint64_t revision) const noexcept;
};

} // namespace consolidator::core
