#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "Core/Domain/ParameterVariant.h"
#include "Core/Domain/State/StatePath.h"

namespace consolidator::core
{

struct ParameterUpdate
{
    StatePath path;
    dsp::ParameterVariant value;
    std::uint64_t revision = 0;
};

struct ParameterUpdateBatch
{
    static constexpr std::size_t kMaximumUpdates = 512;

    std::array<ParameterUpdate, kMaximumUpdates> updates{};
    std::size_t count = 0;
    std::uint64_t revision = 0;
};

} // namespace consolidator::core
