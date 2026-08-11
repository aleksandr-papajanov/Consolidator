#pragma once

#include <variant>

#include "Core/Domain/Commands/StateProtocolCommands.h"
#include "Core/Registry/InstanceRegistry.h"
#include "Core/Routing/StateWriter.h"

namespace consolidator::core
{

class ParameterConstraintResolver;
struct NoCommandResponse
{
};

using CommandResult = std::variant<
    StateResponse,
    StateWriteResult,
    NoCommandResponse>;

// Dispatches protocol commands to state reads or the coordinated write flow.
class CommandRouter
{
public:
    CommandRouter(
        InstanceRegistry& registry,
        const ParameterConstraintResolver& constraintResolver,
        StateWriter& stateWriter) noexcept;

    [[nodiscard]] CommandResult HandleCommand(const Command& command);

private:
    CommandResult Handle(const ReadStateCommand& command);
    CommandResult Handle(const WriteStateCommand& command);
    CommandResult Handle(const ResetDspCommand& command);
    [[nodiscard]] StateResponse HandleReadCommand(const ReadStateCommand& command);

    InstanceRegistry& registry_;
    const ParameterConstraintResolver& constraintResolver_;
    StateWriter& stateWriter_;
};

} // namespace consolidator::core
