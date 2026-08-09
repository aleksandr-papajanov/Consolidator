#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <variant>
#include "Core/State/BankState.h"
#include "Core/Ids/InstanceId.h"
#include "Core/State/StateProtocol.h"

namespace consolidator::core
{

class InstanceState final
{
public:
    static constexpr std::size_t kBankCount = 7;

    InstanceState() noexcept;

    [[nodiscard]] InstanceId GetInstanceId() const noexcept { return instanceId_; }
    [[nodiscard]] dsp::BankId GetSelectedBankId() const noexcept { return selectedBankId_; }
    [[nodiscard]] const BankState& GetBankState(dsp::BankId bankId) const noexcept;

    void ReadState(
        const StatePath& path,
        StateResponseEntries& snapshot) const;

    StateWriteStatus WriteState(const StateEntry& entry, StateResponseEntries& applied);

private:
    friend class InstanceCoordinator;
    friend class StateStore;

    void SetInstanceId(InstanceId instanceId) noexcept { instanceId_ = instanceId; }
    void SetSelectedBankId(dsp::BankId bankId) noexcept { selectedBankId_ = bankId; }
    [[nodiscard]] BankState& GetBankState(dsp::BankId bankId) noexcept;

    InstanceId instanceId_{0};
    std::array<BankState, kBankCount> banks_;
    dsp::BankId selectedBankId_{dsp::BankId::Bank0};
};

inline InstanceState::InstanceState() noexcept
    : banks_{
          BankState{dsp::BankId::Bank0},
          BankState{dsp::BankId::Bank1},
          BankState{dsp::BankId::Bank2},
          BankState{dsp::BankId::Bank3},
          BankState{dsp::BankId::Bank4},
          BankState{dsp::BankId::Bank5},
          BankState{dsp::BankId::Bank6}}
{
}

inline const BankState& InstanceState::GetBankState(dsp::BankId bankId) const noexcept
{
    return banks_[dsp::detail::ToIndex(bankId)];
}

inline void InstanceState::ReadState(
    const StatePath& path,
    StateResponseEntries& snapshot) const
{
    StatePath instancePath;
    instancePath.instanceId = instanceId_;
    instancePath.field = StateField::InstanceId;
    if (path.Matches(instancePath))
    {
        (void)snapshot.TryAppend(StateEntry{instancePath, StateValue{instanceId_}});
    }

    StatePath selectedBankPath = instancePath;
    selectedBankPath.field = StateField::SelectedBank;
    if (path.Matches(selectedBankPath))
    {
        (void)snapshot.TryAppend(StateEntry{selectedBankPath, StateValue{selectedBankId_}});
    }

    for (const auto& bank : banks_)
    {
        StatePath bankPath = instancePath;
        bankPath.field = StateField::BankId;
        bankPath.nodes[0] = static_cast<dsp::RouteNodeId>(
            static_cast<std::uint8_t>(dsp::RouteNodeId::Bank0) +
            dsp::detail::ToIndex(bank.GetBankId()));
        bankPath.depth = 1;
        if (path.Matches(bankPath))
        {
            (void)snapshot.TryAppend(StateEntry{bankPath, StateValue{bank.GetBankId()}});
        }

        if (bank.GetGroupId())
        {
            StatePath groupPath = bankPath;
            groupPath.field = StateField::GroupId;
            if (path.Matches(groupPath))
            {
                (void)snapshot.TryAppend(StateEntry{groupPath, StateValue{*bank.GetGroupId()}});
            }
        }
    }
}

inline StateWriteStatus InstanceState::WriteState(const StateEntry& entry, StateResponseEntries& applied)
{
    if (!entry.path.field)
    {
        return StateWriteStatus::NotHandled;
    }
    if (*entry.path.field == StateField::SelectedBank)
    {
        if (const auto value = std::get_if<dsp::BankId>(&entry.value))
        {
            const auto status = selectedBankId_ == *value
                ? StateWriteStatus::Unchanged
                : StateWriteStatus::Applied;
            selectedBankId_ = *value;
            StateEntry appliedEntry{entry.path, StateValue{*value}};
            appliedEntry.status = status;
            (void)applied.TryAppend(std::move(appliedEntry));
            return status;
        }
        return StateWriteStatus::Rejected;
    }
    if (*entry.path.field == StateField::GroupId && entry.path.depth > 0)
    {
        const auto bankNode = static_cast<std::uint8_t>(entry.path.nodes[0]);
        const auto first = static_cast<std::uint8_t>(dsp::RouteNodeId::Bank0);
        if (bankNode >= first && bankNode < first + kBankCount)
        {
            const auto bankId = static_cast<dsp::BankId>(bankNode - first);
            if (std::holds_alternative<GroupId>(entry.value) ||
                std::holds_alternative<std::monostate>(entry.value))
            {
                if (std::holds_alternative<GroupId>(entry.value))
                {
                    const auto previous = banks_[dsp::detail::ToIndex(bankId)].GetGroupId();
                    const auto& value = std::get<GroupId>(entry.value);
                    const auto status = previous && *previous == value
                        ? StateWriteStatus::Unchanged
                        : StateWriteStatus::Applied;
                    banks_[dsp::detail::ToIndex(bankId)].SetGroupId(value);
                    StateEntry appliedEntry;
                    appliedEntry.path = entry.path;
                    appliedEntry.value = entry.value;
                    appliedEntry.status = status;
                    (void)applied.TryAppend(std::move(appliedEntry));
                    return status;
                }
                else
                {
                    const auto status = banks_[dsp::detail::ToIndex(bankId)].GetGroupId()
                        ? StateWriteStatus::Applied
                        : StateWriteStatus::Unchanged;
                    banks_[dsp::detail::ToIndex(bankId)].SetGroupId(std::nullopt);
                    StateEntry appliedEntry;
                    appliedEntry.path = entry.path;
                    appliedEntry.value = entry.value;
                    appliedEntry.status = status;
                    (void)applied.TryAppend(std::move(appliedEntry));
                    return status;
                }
            }
        }
    }
    return StateWriteStatus::NotHandled;
}

inline BankState& InstanceState::GetBankState(dsp::BankId bankId) noexcept
{
    return banks_[dsp::detail::ToIndex(bankId)];
}

} // namespace consolidator::core
