#pragma once

#include <optional>

#include "Core/Domain/Ids/DspIds.h"
#include "Core/Domain/Ids/GroupId.h"

namespace consolidator::core
{

// Topology metadata for one equalizer bank.
struct BankState
{
    dsp::BankId id;
    std::optional<GroupId> groupId;
};

} // namespace consolidator::core
