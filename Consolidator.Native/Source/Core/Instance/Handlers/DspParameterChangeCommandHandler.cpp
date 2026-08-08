#include "Core/Instance/Handlers/DspParameterChangeCommandHandler.h"

#include "Core/Instance/ConsolidatorInstance.h"
#include "Dsp/Processors/DspChain.h"

namespace consolidator::core
{

void HandleDspParameterChangeCommand(
    ConsolidatorInstance& instance,
    const DspParameterChangeCommand& command)
{
    instance.dspChain_->ApplyParameterChange(
        dsp::RoutedParameterChange{command.route, command.value});
}

} // namespace consolidator::core
