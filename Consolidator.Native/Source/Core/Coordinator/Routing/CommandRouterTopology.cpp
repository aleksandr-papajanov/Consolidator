#include "Core/Coordinator/Routing/CommandRouter.h"

#include <cstdint>
#include <algorithm>
#include <optional>
#include <utility>

#include "Core/Coordinator/Delivery/CommandDeliveryQueue.h"
#include "Core/Instance/ConsolidatorInstance.h"

namespace consolidator::core
{

CommandRouter::CommandRouter(
    InstanceRegistry& registry,
    const StateRouter& stateRouter,
    const ParameterConstraintResolver& constraintResolver,
    CommandDeliveryQueue& deliveryQueue,
    ConcurrentQueue<StateResponse>& coordinatorResponses) noexcept
    : registry_(registry)
    , stateRouter_(stateRouter)
    , constraintResolver_(constraintResolver)
    , deliveryQueue_(deliveryQueue)
    , coordinatorResponses_(coordinatorResponses)
{
}

std::optional<dsp::BankId> TryGetBankId(
    const StatePath& path) noexcept;

bool CommandRouter::ApplyTopologyWrite(
    InstanceId sourceInstanceId,
    const StateEntry& entry,
    StateResponseEntries& applied,
    std::vector<BankAddress>& affectedBanks)
{
    auto* source =
        registry_.FindInstance(sourceInstanceId);

    if (source == nullptr || !entry.path.field)
    {
        return false;
    }

    std::optional<BankAddress> changedBank;
    std::optional<GroupId> previousGroup;

    if (*entry.path.field == StateField::GroupId)
    {
        const auto bankId =
            TryGetBankId(entry.path);

        if (!bankId)
        {
            return false;
        }

        changedBank =
            BankAddress{sourceInstanceId, *bankId};

        const auto& sourceState =
            static_cast<const InstanceState&>(source->GetState());
        previousGroup =
            sourceState.GetBankState(*bankId).GetGroupId();
    }

    const auto status =
        source->GetState().WriteState(entry, applied);

    if (status == StateWriteStatus::NotHandled)
    {
        return false;
    }

    if (status == StateWriteStatus::Rejected)
    {
        auto rejected = entry;
        rejected.status = StateWriteStatus::Rejected;
        (void)applied.TryAppend(std::move(rejected));
        return true;
    }

    if (changedBank)
    {
        const auto appendUnique = [&affectedBanks](BankAddress bank)
        {
            if (std::find(affectedBanks.begin(), affectedBanks.end(), bank) == affectedBanks.end())
            {
                affectedBanks.push_back(bank);
            }
        };

        if (previousGroup)
        {
            for (const auto& member : registry_.FindGroupMembers(*previousGroup))
            {
                appendUnique(member);
            }
        }

        const auto& sourceState =
            static_cast<const InstanceState&>(source->GetState());
        const auto nextGroup =
            sourceState.GetBankState(changedBank->bankId).GetGroupId();

        if (nextGroup)
        {
            for (const auto& member : registry_.FindGroupMembers(*nextGroup))
            {
                appendUnique(member);
            }
        }

        appendUnique(*changedBank);

        registry_.CacheBankGroup(
            *changedBank,
            previousGroup,
            nextGroup);
    }

    return true;
}

std::optional<dsp::BankId> TryGetBankId(
    const StatePath& path) noexcept
{
    if (path.depth == 0)
    {
        return std::nullopt;
    }

    const auto node =
        static_cast<std::uint8_t>(path.nodes[0]);

    const auto first =
        static_cast<std::uint8_t>(
            dsp::RouteNodeId::Bank0);

    const auto last =
        first + InstanceState::kBankCount - 1;

    if (node < first || node > last)
    {
        return std::nullopt;
    }

    return static_cast<dsp::BankId>(node - first);
}

} // namespace consolidator::core
