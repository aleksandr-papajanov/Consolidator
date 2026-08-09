#pragma once

#include "Core/State/StateProtocol.h"

namespace consolidator::core
{

class IStateSource
{
public:
    virtual ~IStateSource() = default;

    virtual void AppendState(
        const StatePath& path,
        StateSnapshot& snapshot) const = 0;
};

} // namespace consolidator::core
