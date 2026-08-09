#include "Core/Instance/ConsolidatorInstance.h"

#include <algorithm>
#include <utility>

#include "Core/Coordinator/InstanceCoordinator.h"
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
    commandQueue_.Process(*this, responseQueue_);
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

bool ConsolidatorInstance::EnqueueLocalCommand(Command command)
{
    return commandQueue_.TryEnqueue(std::move(command));
}

void ConsolidatorInstance::RecordLocalQueueOverflow() noexcept
{
    commandQueue_.RecordOverflow();
}

std::optional<StateResponse> ConsolidatorInstance::TryDequeueResponse()
{
    return responseQueue_.TryDequeue();
}

} // namespace consolidator::core
