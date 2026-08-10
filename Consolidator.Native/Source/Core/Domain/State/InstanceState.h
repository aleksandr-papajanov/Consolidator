#pragma once

#include <array>
#include <cstddef>

#include "Core/Domain/Ids/DspIds.h"
#include "Core/Domain/Ids/InstanceId.h"
#include "Core/Domain/State/BankState.h"

namespace consolidator::core
{

struct InstanceState
{
    static constexpr std::size_t kBankCount = 7;

    InstanceId instanceId{0};
    dsp::BankId selectedBankId{dsp::BankId::Bank0};
    std::array<BankState, kBankCount> banks{
        BankState{dsp::BankId::Bank0},
        BankState{dsp::BankId::Bank1},
        BankState{dsp::BankId::Bank2},
        BankState{dsp::BankId::Bank3},
        BankState{dsp::BankId::Bank4},
        BankState{dsp::BankId::Bank5},
        BankState{dsp::BankId::Bank6}};
};

} // namespace consolidator::core
