#include "Core/Instance/ConsolidatorInstance.h"

#include <algorithm>
#include <type_traits>
#include <utility>

#include "Core/Coordinator/InstanceCoordinator.h"
#include "Core/Instance/Handlers/DspParameterChangeCommandHandler.h"
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
    ProcessCommandQueue();
    dspChain_->Process(mainInput, referenceOutput, mainOutput, frameCount, kChannelCount);
    std::copy_n(referenceInput, frameCount * kChannelCount, referenceOutput);
}

void ConsolidatorInstance::EnqueueCommand(Command command)
{
    InstanceCoordinator::Get().EnqueueCommand(state_.GetInstanceId(), std::move(command));
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

const InstanceState& ConsolidatorInstance::GetState() const noexcept
{
    return state_;
}

void ConsolidatorInstance::HandleCommand(const Command& command)
{
    std::visit(
        [this](const auto& typedCommand)
        {
            using CommandType = std::decay_t<decltype(typedCommand)>;
            if constexpr (std::is_same_v<CommandType, DspParameterChangeCommand>)
            {
                HandleDspParameterChangeCommand(*this, typedCommand);
            }
        },
        command);
}

} // namespace consolidator::core
