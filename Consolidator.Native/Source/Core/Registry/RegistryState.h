#pragma once

#include "Core/Registry/RegistrySnapshot.h"

namespace consolidator::core
{

class InstanceRegistry;

class RegistryState final
{
public:
    [[nodiscard]] bool Refresh(const InstanceRegistry& registry);

    [[nodiscard]] const RegistrySnapshot& Get() const noexcept
    {
        return snapshot_;
    }

private:
    RegistrySnapshot snapshot_;
};

} // namespace consolidator::core
