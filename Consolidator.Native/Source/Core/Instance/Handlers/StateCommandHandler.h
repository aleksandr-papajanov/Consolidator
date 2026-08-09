#pragma once

#include "Core/Commands/Commands.h"

namespace consolidator::core
{

class ConsolidatorInstance;

void HandleStateCommand(
    ConsolidatorInstance& instance,
    const StateCommand& command);

} // namespace consolidator::core
