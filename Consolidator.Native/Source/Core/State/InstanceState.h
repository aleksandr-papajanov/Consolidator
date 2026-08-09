#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include "Core/State/BankState.h"
#include "Core/Instance/InstanceId.h"
#include "Core/State/IStateNode.h"

namespace consolidator::core
{

class InstanceState final : public IStateNode
{
public:
    static constexpr std::size_t kBankCount = 7;

    InstanceState() noexcept;

    [[nodiscard]] InstanceId GetInstanceId() const noexcept { return instanceId_; }
    [[nodiscard]] dsp::BankId GetSelectedBankId() const noexcept { return selectedBankId_; }
    [[nodiscard]] const BankState& GetBankState(dsp::BankId bankId) const noexcept;

    void ReadState(
        const StatePath& path,
        StateResponseEntries& snapshot) const override;

    bool WriteState(const StateEntry&, StateResponseEntries&) override
    {
        return false;
    }

private:
    friend class InstanceCoordinator;

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
        bankPath.bankNode = static_cast<dsp::RouteNodeId>(
            static_cast<std::uint8_t>(dsp::RouteNodeId::Bank0) +
            dsp::detail::ToIndex(bank.GetBankId()));
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

inline BankState& InstanceState::GetBankState(dsp::BankId bankId) noexcept
{
    return banks_[dsp::detail::ToIndex(bankId)];
}

} // namespace consolidator::core
