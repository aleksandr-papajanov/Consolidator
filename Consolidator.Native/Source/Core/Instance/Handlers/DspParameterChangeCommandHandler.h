#pragma once

#include "Core/Commands/Commands.h"

namespace consolidator::core
{

class ConsolidatorInstance;

void HandleDspParameterChangeCommand(
    ConsolidatorInstance& instance,
    const DspParameterChangeCommand& command);

} // namespace consolidator::core
