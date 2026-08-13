#pragma once

#include <array>
#include <cstddef>
#include <string>

#include "Core/Domain/Ids/DspIds.h"
#include "Core/Domain/Ids/InstanceId.h"
#include "Core/Domain/State/BankState.h"
#include "Core/Domain/State/InstanceAudibilityState.h"

namespace consolidator::core
{

// Topology state owned by one instance, including its selected bank and groups.
struct InstanceState
{
    static constexpr std::size_t kBankCount = 7;

    InstanceId instanceId{0};
    std::string label;
    dsp::BankId selectedBankId{dsp::BankId::Bank0};
    std::array<BankState, kBankCount> banks{
        BankState{dsp::BankId::Bank0},
        BankState{dsp::BankId::Bank1},
        BankState{dsp::BankId::Bank2},
        BankState{dsp::BankId::Bank3},
        BankState{dsp::BankId::Bank4},
        BankState{dsp::BankId::Bank5},
        BankState{dsp::BankId::Bank6}};
    dsp::InstanceAudibilityState audibility;
};

} // namespace consolidator::core
