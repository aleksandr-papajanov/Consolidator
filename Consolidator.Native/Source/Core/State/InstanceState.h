#pragma once

#include <array>
#include <cstddef>
#include <optional>

#include "Core/Groups/GroupId.h"
#include "Core/Instance/InstanceId.h"
#include "Dsp/Parameters/DspIds.h"

namespace consolidator::core
{

class BankState
{
public:
    explicit constexpr BankState(dsp::BankId bankId) noexcept
        : bankId_(bankId)
    {
    }

    [[nodiscard]] constexpr dsp::BankId GetBankId() const noexcept { return bankId_; }
    [[nodiscard]] const std::optional<GroupId>& GetGroupId() const noexcept { return groupId_; }

private:
    friend class InstanceCoordinator;

    void SetGroupId(std::optional<GroupId> groupId) noexcept { groupId_ = groupId; }

    dsp::BankId bankId_;
    std::optional<GroupId> groupId_;
};

class InstanceState
{
public:
    static constexpr std::size_t kBankCount = 7;

    InstanceState() noexcept;

    [[nodiscard]] InstanceId GetInstanceId() const noexcept { return instanceId_; }
    [[nodiscard]] dsp::BankId GetSelectedBankId() const noexcept { return selectedBankId_; }
    [[nodiscard]] const BankState& GetBankState(dsp::BankId bankId) const noexcept;

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

inline BankState& InstanceState::GetBankState(dsp::BankId bankId) noexcept
{
    return banks_[dsp::detail::ToIndex(bankId)];
}

} // namespace consolidator::core
