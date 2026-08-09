#include "Core/Instance/ConsolidatorInstance.h"

#include <type_traits>
#include <utility>

#include "Core/Coordinator/InstanceCoordinator.h"
#include "Core/Instance/Handlers/ChangeDspParameterCommandHandler.h"
#include "Core/Instance/Handlers/ReadStateCommandHandler.h"
#include "Dsp/DspChainBuilder.h"
#include "Dsp/Processors/DspChain.h"

namespace consolidator::core
{

ConsolidatorInstance::ConsolidatorInstance()
    : dspChain_(dsp::DspChainBuilder{}.BuildStandardChain())
{
    InstanceCoordinator::Get().RegisterInstance(*this);
}

ConsolidatorInstance::~ConsolidatorInstance()
{
    InstanceCoordinator::Get().UnregisterInstance(state_.GetInstanceId());
}

void ConsolidatorInstance::Process(const double* mainInput,
                                   const double* referenceInput,
                                   double* mainOutput,
                                   double* referenceOutput,
                                   std::size_t frameCount)
{
    if (pendingResponse_)
    {
        if (responseQueue_.TryEnqueue(std::move(*pendingResponse_)))
        {
            pendingResponse_.reset();
        }
    }

    ProcessCommandQueue();
    dspChain_->Process(mainInput, referenceOutput, mainOutput, frameCount, kChannelCount);
    std::copy_n(referenceInput, frameCount * kChannelCount, referenceOutput);
}

void ConsolidatorInstance::EnqueueCommand(Command command)
{
    InstanceCoordinator::Get().EnqueueCommand(state_.GetInstanceId(), std::move(command));
}

InstanceId ConsolidatorInstance::GetInstanceId() const noexcept
{
    return state_.GetInstanceId();
}

bool ConsolidatorInstance::EnqueueLocalCommand(Command command)
{
    return commandQueue_.TryEnqueue(std::move(command));
}

void ConsolidatorInstance::RecordLocalQueueOverflow() noexcept
{
    localQueueOverflowCount_.fetch_add(1, std::memory_order_relaxed);
}

void ConsolidatorInstance::ProcessCommandQueue()
{
    while (const auto command = commandQueue_.TryDequeue())
    {
        HandleCommand(*command);
    }
}

void ConsolidatorInstance::HandleCommand(const Command& command)
{
    std::visit(
        [this](const auto& typedCommand)
        {
            using CommandType = std::decay_t<decltype(typedCommand)>;
            if constexpr (std::is_same_v<CommandType, ChangeDspParameterCommand>)
            {
                HandleChangeDspParameterCommand(*this, typedCommand);
            }
            else if constexpr (std::is_same_v<CommandType, ReadStateCommand>)
            {
                HandleReadStateCommand(*this, typedCommand);
            }
        },
        command);
}

std::optional<StateResponse> ConsolidatorInstance::TryDequeueResponse()
{
    return responseQueue_.TryDequeue();
}

void ConsolidatorInstance::RecordResponseQueueOverflow() noexcept
{
    responseQueueOverflowCount_.fetch_add(1, std::memory_order_relaxed);
}

} // namespace consolidator::core
