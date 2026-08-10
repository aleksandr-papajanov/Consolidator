#pragma once

#include "Core/Domain/Commands/StateProtocolCommands.h"
#include "Core/Registry/InstanceRegistry.h"
#include "Core/Queues/ConcurrentQueue.h"

namespace consolidator::core
{

class ParameterConstraintResolver;
class StateWriter;

class CommandRouter
{
public:
    CommandRouter(
        InstanceRegistry& registry,
        const ParameterConstraintResolver& constraintResolver,
        StateWriter& stateWriter,
        ConcurrentQueue<StateResponse>& coordinatorResponses) noexcept;

    void HandleCommand(const Command& command);

private:
    void Handle(const ReadStateCommand& command);
    void Handle(const WriteStateCommand& command);
    void HandleReadCommand(const ReadStateCommand& command);

    InstanceRegistry& registry_;
    const ParameterConstraintResolver& constraintResolver_;
    StateWriter& stateWriter_;
    ConcurrentQueue<StateResponse>& coordinatorResponses_;
};

} // namespace consolidator::core
