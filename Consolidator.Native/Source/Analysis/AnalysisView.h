#pragma once

#include "Core/Domain/Ids/DspIds.h"
#include "Core/Domain/Ids/InstanceId.h"

namespace consolidator::analysis
{

// Identifies the instance and bank currently rendered by the global analyzer.
struct AnalysisView
{
    core::InstanceId instanceId{0};
    dsp::BankId bankId{dsp::BankId::Bank0};
};

constexpr bool operator==(const AnalysisView& lhs, const AnalysisView& rhs) noexcept
{
    return lhs.instanceId == rhs.instanceId && lhs.bankId == rhs.bankId;
}

} // namespace consolidator::analysis
