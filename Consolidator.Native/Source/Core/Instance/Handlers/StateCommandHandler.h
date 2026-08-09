#pragma once

#include "Core/Commands/Commands.h"
#include "Core/Notifications/Notifications.h"

namespace consolidator::core
{

class ConsolidatorInstance;

[[nodiscard]] StateResponse HandleStateCommand(
    ConsolidatorInstance& instance,
    const StateCommand& command);

} // namespace consolidator::core
