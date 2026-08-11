#pragma once

#include <vector>

#include "Core/Instance/Queues/RuntimeControlUpdates.h"
#include "Core/Registry/InstanceRegistry.h"
#include "Core/Routing/GroupGraph.h"

namespace consolidator::core
{

// Resolves output solo scopes across all live instances.
class InstanceAudibilityResolver final
{
public:
    InstanceAudibilityResolver(
        const InstanceRegistry& registry,
        const GroupGraph& groups) noexcept
        : registry_(registry)
        , groups_(groups)
    {
    }

    void Resolve(std::vector<RuntimeControlUpdate>& updates) const;

private:
    const InstanceRegistry& registry_;
    const GroupGraph& groups_;
};

} // namespace consolidator::core
