#pragma once

#include "Core/State/StateProtocol.h"

namespace consolidator::core
{

class IStateNode
{
public:
    virtual ~IStateNode() = default;

    virtual void ReadState(const StatePath& query, StateResponseEntries& output) const = 0;
    virtual bool WriteState(const StateEntry& entry, StateResponseEntries& applied) = 0;
};

} // namespace consolidator::core
