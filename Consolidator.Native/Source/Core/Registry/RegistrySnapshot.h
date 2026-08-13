#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "Core/Domain/Ids/DspIds.h"
#include "Core/Domain/Ids/GroupId.h"
#include "Core/Domain/Ids/InstanceId.h"
#include "Core/Registry/BankAddress.h"

namespace consolidator::core
{

struct RegistryBankSnapshot
{
    dsp::BankId bankId{dsp::BankId::Bank0};
    std::optional<GroupId> groupId;

    friend bool operator==(const RegistryBankSnapshot&, const RegistryBankSnapshot&) = default;
};

struct RegistryInstanceSnapshot
{
    InstanceId instanceId{0};
    std::string label;
    dsp::BankId selectedBankId{dsp::BankId::Bank0};
    std::vector<RegistryBankSnapshot> banks;

    friend bool operator==(const RegistryInstanceSnapshot&, const RegistryInstanceSnapshot&) = default;
};

struct RegistryGroupSnapshot
{
    GroupId groupId{0};
    std::vector<BankAddress> members;

    friend bool operator==(const RegistryGroupSnapshot&, const RegistryGroupSnapshot&) = default;
};

struct RegistrySnapshot
{
    std::uint64_t revision{0};
    std::vector<RegistryInstanceSnapshot> instances;
    std::vector<RegistryGroupSnapshot> groups;
};

} // namespace consolidator::core
