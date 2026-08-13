#pragma once

#include <variant>

#include "Core/Domain/Commands/StateProtocolCommands.h"
#include "Core/Registry/InstanceRegistry.h"
#include "Core/Routing/StateWriter.h"

namespace consolidator::core
{

class ParameterConstraintResolver;
class RegistryState;
struct NoCommandResponse
{
};

using CommandResult = std::variant<
    StateResponse,
    StateWriteResult,
    ActionResponse,
    RegistryResponse,
    NoCommandResponse>;

// Dispatches protocol commands to state reads or the coordinated write flow.
class CommandRouter
{
public:
    CommandRouter(
        InstanceRegistry& registry,
        const RegistryState& registryState,
        const ParameterConstraintResolver& constraintResolver,
        StateWriter& stateWriter) noexcept;

    [[nodiscard]] CommandResult HandleCommand(const Command& command);

private:
    CommandResult Handle(const ReadStateCommand& command);
    CommandResult Handle(const WriteStateCommand& command);
    CommandResult Handle(const ResetDspCommand& command);
    CommandResult Handle(const ReadRegistryCommand& command);
    [[nodiscard]] StateResponse HandleReadCommand(const ReadStateCommand& command);

    InstanceRegistry& registry_;
    const RegistryState& registryState_;
    const ParameterConstraintResolver& constraintResolver_;
    StateWriter& stateWriter_;
};

} // namespace consolidator::core
