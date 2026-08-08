#include "Core/Commands/Commands.h"
#include "Core/Coordinator/InstanceCoordinator.h"
#include "Core/Instance/ConsolidatorInstance.h"

#include <cassert>

int main()
{
    consolidator::core::ConsolidatorInstance instance;

    assert(instance.GetState().GetSelectedBankId() == consolidator::dsp::BankId::Bank0);
    assert(!instance.GetState().GetBankState(consolidator::dsp::BankId::Bank0).GetGroupId());

    instance.EnqueueCommand(consolidator::core::ChangeDspParameterCommand{
        consolidator::dsp::ParameterRoute{
            consolidator::dsp::DeviceId::MainInputGain,
            consolidator::dsp::ParameterId::Gain},
        consolidator::dsp::ParameterValue{6.0f}});

    return 0;
}
