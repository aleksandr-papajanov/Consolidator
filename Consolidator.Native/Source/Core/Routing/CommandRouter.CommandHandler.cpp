#include "Core/Routing/CommandRouter.h"

#include "Core/Registry/RegistryState.h"

namespace consolidator::core
{

CommandRouter::CommandRouter(
    InstanceRegistry& registry,
    const RegistryState& registryState,
    const ParameterConstraintResolver& constraintResolver,
    StateWriter& stateWriter) noexcept
    : registry_(registry)
    , registryState_(registryState)
    , constraintResolver_(constraintResolver)
    , stateWriter_(stateWriter)
{
}

CommandResult CommandRouter::Handle(const ReadRegistryCommand& command)
{
    return RegistryResponse{
        command.requestId,
        command.instanceId,
        registryState_.Get()};
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
