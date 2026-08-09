#include "Core/Coordinator/StateRouter.h"

#include <cstdint>

#include "Core/State/InstanceState.h"
#include "Core/Instance/ConsolidatorInstance.h"

namespace consolidator::core
{

namespace
{

dsp::RouteNodeId ToRouteNodeId(dsp::BankId bankId) noexcept
{
    return static_cast<dsp::RouteNodeId>(
        static_cast<std::uint8_t>(dsp::RouteNodeId::Bank0) +
        dsp::detail::ToIndex(bankId));
}

} // namespace

StateEntry StateRouter::ForBank(StateEntry entry, dsp::BankId bankId)
{
    if (entry.path.depth == 0)
    {
        entry.path.depth = 1;
    }
    entry.path.nodes[0] = ToRouteNodeId(bankId);
    return entry;
}

std::vector<BankAddress> StateRouter::ResolveTargets(
    InstanceId sourceInstanceId,
    const StatePath& path) const
{
    if (!path.deviceId || *path.deviceId != dsp::DeviceId::Equalizer)
    {
        return {};
    }

    const auto* source = registry_.FindInstance(sourceInstanceId);
    if (source == nullptr)
    {
        return {};
    }

    const auto sourceBankId = source->GetState().GetSelectedBankId();
    const auto groupId = source->GetState().GetBankState(sourceBankId).GetGroupId();
    if (!groupId)
    {
        return {BankAddress{sourceInstanceId, sourceBankId}};
    }

    const auto members = registry_.FindGroupMembers(*groupId);
    return {members.begin(), members.end()};
}

} // namespace consolidator::core
