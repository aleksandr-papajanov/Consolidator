#include "Core/Routing/CommandRouter.h"

#include <type_traits>
#include <utility>

namespace consolidator::core
{

CommandRouter::CommandRouter(
    InstanceRegistry& registry,
    const StateRouter& stateRouter,
    const ParameterConstraintResolver& constraintResolver,
    ConcurrentQueue<StateResponse>& coordinatorResponses) noexcept
    : registry_(registry)
    , stateRouter_(stateRouter)
    , constraintResolver_(constraintResolver)
    , coordinatorResponses_(coordinatorResponses)
{
}

void CommandRouter::HandleCommand(const InstanceCommand& command)
{
    std::visit(
        [this, sourceInstanceId = command.sourceInstanceId](
            const auto& typedCommand)
        {
            using CommandType = std::decay_t<decltype(typedCommand)>;
            if constexpr (std::is_same_v<CommandType, StateCommand>)
            {
                HandleStateCommand(sourceInstanceId, typedCommand);
            }
        },
        command.command);
}

} // namespace consolidator::core
