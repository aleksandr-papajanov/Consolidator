#include "Core/Instance/Handlers/ReadStateCommandHandler.h"

#include <utility>

#include "Core/Instance/ConsolidatorInstance.h"
#include "Dsp/Processors/DspChain.h"

namespace consolidator::core
{

void HandleReadStateCommand(
    ConsolidatorInstance& instance,
    const ReadStateCommand& command)
{
    StateResponse response{command.requestId};
    auto path = command.path;
    path.instanceId = instance.state_.GetInstanceId();
    instance.state_.AppendState(path, response.snapshot);
    instance.dspChain_->AppendState(path, response.snapshot);

    if (!instance.responseQueue_.TryEnqueue(response))
    {
        instance.pendingResponse_ = std::move(response);
        instance.RecordResponseQueueOverflow();
    }
}

} // namespace consolidator::core
