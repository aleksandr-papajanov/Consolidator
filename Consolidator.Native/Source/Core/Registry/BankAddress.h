#pragma once

#include "Core/Domain/Ids/DspIds.h"
#include "Core/Domain/Ids/InstanceId.h"

namespace consolidator::core
{

// Identifies a bank together with the instance that owns it.
struct BankAddress
{
    InstanceId instanceId;
    dsp::BankId bankId;

    friend bool operator==(const BankAddress&, const BankAddress&) = default;
};

} // namespace consolidator::core
