#pragma once

#include <vector>

#include "Core/Domain/State/StateStore.h"
#include "Core/Domain/State/ChainState.h"
#include "Core/Instance/Queues/RuntimeUpdateMailbox.h"
#include "Core/Routing/ProcessingState.h"

namespace consolidator::core
{

struct RuntimeResolution
{
    std::vector<RuntimeControlUpdate> controls;
};

// Resolves authoritative solo/bypass state into audio-thread processing state.
class ProcessingStateResolver final
{
public:
    // Produces a complete internal processing snapshot; active is never stored in StateStore.
    void Resolve(
        InstanceId instanceId,
        const StateStore& stateStore,
        RuntimeResolution& resolution) const;

private:
    static void AppendActiveUpdate(
        const StatePath& path,
        bool active,
        std::vector<RuntimeControlUpdate>& updates);

    static void AppendMonitoringUpdate(
        const StatePath& path,
        bool enabled,
        std::vector<RuntimeControlUpdate>& updates);
};

} // namespace consolidator::core
