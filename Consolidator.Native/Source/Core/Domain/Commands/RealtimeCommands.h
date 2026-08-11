#pragma once

#include <variant>

#include "Core/Domain/State/StatePath.h"

namespace consolidator::core
{

// Ordered audio-thread event for runtime actions that must not be coalesced.
struct ResetRuntimeCommand
{
    StatePath target;
};

using RealtimeCommand = std::variant<ResetRuntimeCommand>;

} // namespace consolidator::core
