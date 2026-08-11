#pragma once

#include <vector>

#include "Core/Domain/State/StateStore.h"
#include "Core/Domain/State/ChainState.h"
#include "Core/Instance/Queues/RuntimeUpdateMailbox.h"

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
    [[nodiscard]] bool ResolveChain(
        InstanceId instanceId,
        const StateStore& stateStore,
        std::vector<RuntimeControlUpdate>& updates,
        bool& saturatorActive,
        bool& compressorActive) const;

    void ResolveEqualizer(
        InstanceId instanceId,
        const StateStore& stateStore,
        bool equalizerActive,
        std::vector<RuntimeControlUpdate>& updates) const;

    void ResolveBankFilters(
        InstanceId instanceId,
        dsp::BankId bankId,
        const dsp::EqualizerBankState& bank,
        bool bankActive,
        std::vector<RuntimeControlUpdate>& updates) const;

    void ResolveDetectorFilters(
        InstanceId instanceId,
        const StateStore& stateStore,
        bool saturatorActive,
        bool compressorActive,
        std::vector<RuntimeControlUpdate>& updates) const;

    void ResolveMonitoring(
        InstanceId instanceId,
        const StateStore& stateStore,
        std::vector<RuntimeControlUpdate>& updates) const;

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
