#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "Core/Domain/State/StatePath.h"

namespace consolidator::core
{

enum class RuntimeProperty : std::uint8_t
{
    Active,
    Listen,
    OutputEnabled
};

struct RuntimeControlKey
{
    StatePath target;
    RuntimeProperty property = RuntimeProperty::Active;

    friend constexpr bool operator==(
        const RuntimeControlKey&,
        const RuntimeControlKey&) noexcept = default;
};

struct RuntimeControlUpdate
{
    StatePath target;
    RuntimeProperty property = RuntimeProperty::Active;
    bool value = false;
    std::uint64_t revision = 0;
};

struct RuntimeControlBatch
{
    static constexpr std::size_t kMaximumUpdates = 136;

    std::array<RuntimeControlUpdate, kMaximumUpdates> updates{};
    std::size_t count = 0;
    std::uint64_t revision = 0;
};

} // namespace consolidator::core
