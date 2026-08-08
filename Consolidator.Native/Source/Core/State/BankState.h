#pragma once

#include <array>
#include <cstddef>
#include <optional>

#include "Core/Groups/GroupId.h"
#include "Core/Parameters/DspIds.h"

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

} // namespace consolidator::core
