#pragma once

#include "Core/Commands/Commands.h"

namespace consolidator::core
{

class ConsolidatorInstance;

void HandleReadStateCommand(
    ConsolidatorInstance& instance,
    const ReadStateCommand& command);

} // namespace consolidator::core
