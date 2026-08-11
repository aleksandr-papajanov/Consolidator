#include "Core/Routing/CommandRouter.h"

namespace consolidator::core
{

CommandRouter::CommandRouter(
    InstanceRegistry& registry,
    const ParameterConstraintResolver& constraintResolver,
    StateWriter& stateWriter) noexcept
    : registry_(registry)
    , constraintResolver_(constraintResolver)
    , stateWriter_(stateWriter)
{
}

CommandResult CommandRouter::HandleCommand(const Command& command)
{
    return std::visit(
        [this](const auto& typedCommand)
        {
            return Handle(typedCommand);
        },
        command);
}

} // namespace consolidator::core
