#include "Core/Domain/Commands/StateProtocolCommands.h"
#include "Core/Coordinator/InstanceCoordinator.h"
#include "Core/Instance/ConsolidatorInstance.h"

#include <cassert>

int main()
{
    consolidator::core::ConsolidatorInstance instance;


    consolidator::core::StateRequestEntries entries;
    consolidator::core::StatePath path;
    path.field = consolidator::core::StateField::DspParameter;
    path.deviceId = consolidator::dsp::DeviceId::MainInputGain;
    path.parameterId = consolidator::dsp::ParameterId::Gain;
    assert(entries.TryAppend({path, 6.0f}));
    instance.EnqueueCommand(consolidator::core::WriteStateCommand{
        .requestId = 1,
        .entries = entries});

    return 0;
}
