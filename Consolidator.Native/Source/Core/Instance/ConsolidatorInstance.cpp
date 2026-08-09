#include "Core/Instance/ConsolidatorInstance.h"

#include <type_traits>
#include <utility>

#include "Core/Coordinator/InstanceCoordinator.h"
#include "Core/Instance/Handlers/StateCommandHandler.h"
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
    while (pendingResponseCount_ > 0)
    {
        if (!responseQueue_.TryEnqueue(std::move(pendingResponses_[0])))
        {
            break;
        }

        for (std::size_t index = 1; index < pendingResponseCount_; ++index)
        {
            pendingResponses_[index - 1] = std::move(pendingResponses_[index]);
        }
        --pendingResponseCount_;
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

dsp::DspChain& ConsolidatorInstance::GetDspChain() noexcept
{
    return *dspChain_;
}

void ConsolidatorInstance::QueueStateResponse(StateResponse response) noexcept
{
    if (!responseQueue_.TryEnqueue(response))
    {
        StorePendingResponse(std::move(response));
    }
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
            if constexpr (std::is_same_v<CommandType, StateCommand>)
            {
                HandleStateCommand(*this, typedCommand);
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

void ConsolidatorInstance::StorePendingResponse(StateResponse response) noexcept
{
    if (pendingResponseCount_ < pendingResponses_.size())
    {
        pendingResponses_[pendingResponseCount_++] = std::move(response);
    }

    RecordResponseQueueOverflow();
}

} // namespace consolidator::core
