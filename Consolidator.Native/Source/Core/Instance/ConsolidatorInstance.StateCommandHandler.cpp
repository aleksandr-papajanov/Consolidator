#include "Core/Instance/ConsolidatorInstance.h"

#include <utility>

#include "Core/Coordinator/InstanceCoordinator.h"

namespace consolidator::core
{

void ConsolidatorInstance::HandleStateCommand(StateCommand command)
{
    InstanceCoordinator::Get().EnqueueStateCommand(
        state_.GetInstanceId(),
        std::move(command));
}

} // namespace consolidator::core
