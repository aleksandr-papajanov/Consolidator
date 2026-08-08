#include "Core/Instance/Handlers/ChangeDspParameterCommandHandler.h"

#include "Core/Instance/ConsolidatorInstance.h"
#include "Dsp/Processors/DspChain.h"

namespace consolidator::core
{

void HandleChangeDspParameterCommand(
    ConsolidatorInstance& instance,
    const ChangeDspParameterCommand& command)
{
    instance.dspChain_->ApplyParameterChange(
        dsp::RoutedParameterChange{command.route, command.value});
}

} // namespace consolidator::core
