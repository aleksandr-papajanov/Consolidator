#pragma once

#include "Core/Commands/Commands.h"

namespace consolidator::core
{

class ConsolidatorInstance;

void HandleChangeDspParameterCommand(
    ConsolidatorInstance& instance,
    const ChangeDspParameterCommand& command);

} // namespace consolidator::core
