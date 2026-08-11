#include "Core/Instance/ConsolidatorInstance.h"

#include <utility>

#include "Core/Coordinator/InstanceCoordinator.h"

namespace consolidator::core
{

void ConsolidatorInstance::EnqueueCommand(ReadStateCommand command)
{
    command.instanceId = GetInstanceId();
    InstanceCoordinator::Get().EnqueueCommand(Command{std::move(command)});
}

void ConsolidatorInstance::EnqueueCommand(WriteStateCommand command)
{
    command.instanceId = GetInstanceId();
    InstanceCoordinator::Get().EnqueueCommand(Command{std::move(command)});
}

void ConsolidatorInstance::EnqueueCommand(ResetDspCommand command)
{
    command.instanceId = GetInstanceId();
    InstanceCoordinator::Get().EnqueueCommand(Command{std::move(command)});
}

std::optional<CommandResponse> ConsolidatorInstance::TryDequeueResponse()
{
    return responseQueue_.TryDequeue();
}

bool ConsolidatorInstance::SetResponseNotifier(ResponseNotifier notifier)
{
    if (initialized_)
    {
        return false;
    }
    responseNotifier_ = std::make_shared<ResponseNotifierState>(
        std::move(notifier));
    return true;
}

void ConsolidatorInstance::EnqueueResponse(CommandResponse response)
{
    responseQueue_.Enqueue(std::move(response));
}

ConsolidatorInstance::ResponseNotifierHandle
ConsolidatorInstance::GetResponseNotifierHandle() const noexcept
{
    return responseNotifier_;
}

void ConsolidatorInstance::NotifyResponseAvailable(
    ResponseNotifierHandle notifier)
{
    if (!notifier)
    {
        return;
    }
    std::lock_guard lock{notifier->mutex};
    if (notifier->active && notifier->callback)
    {
        notifier->callback();
    }
}

void ConsolidatorInstance::ShutdownResponseNotifier() noexcept
{
    if (!responseNotifier_)
    {
        return;
    }
    std::lock_guard lock{responseNotifier_->mutex};
    responseNotifier_->active = false;
}

void ConsolidatorInstance::EnqueueParameterUpdates(
    std::span<const ParameterUpdate> updates)
{
    runtimeUpdateMailbox_.EnqueueParameters(updates);
}

void ConsolidatorInstance::EnqueueRuntimeUpdates(
    std::span<const RuntimeControlUpdate> updates)
{
    runtimeUpdateMailbox_.EnqueueRuntimeControls(updates);
}

bool ConsolidatorInstance::EnqueueRealtimeCommand(const StatePath& target)
{
    return realtimeCommandQueue_.TryEnqueue(
        RealtimeCommand{ResetRuntimeCommand{target}});
}

} // namespace consolidator::core
