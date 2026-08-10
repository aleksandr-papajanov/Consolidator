#include "Core/Routing/CommandRouter.h"

namespace consolidator::core
{

CommandRouter::CommandRouter(
    InstanceRegistry& registry,
    const ParameterConstraintResolver& constraintResolver,
    StateWriter& stateWriter,
    ConcurrentQueue<StateResponse>& coordinatorResponses) noexcept
    : registry_(registry)
    , constraintResolver_(constraintResolver)
    , stateWriter_(stateWriter)
    , coordinatorResponses_(coordinatorResponses)
{
}

void CommandRouter::HandleCommand(const Command& command)
{
    std::visit(
        [this](const auto& typedCommand)
        {
            Handle(typedCommand);
        },
        command);
}

} // namespace consolidator::core
