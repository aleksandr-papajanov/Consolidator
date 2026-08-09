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
    PublishParameterStateView();
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
    PublishParameterStateView();
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

bool ConsolidatorInstance::ReadPublishedParameterState(
    const StatePath& path,
    StateEntry& result) const
{
    auto query = path;
    query.instanceId = state_.GetInstanceId();
    const auto view = std::atomic_load_explicit(
        &publishedParameterState_,
        std::memory_order_acquire);
    if (!view)
    {
        return false;
    }
    for (std::size_t index = 0; index < view->entries.size; ++index)
    {
        if (view->entries.entries[index].path.Matches(query))
        {
            result = view->entries.entries[index];
            return true;
        }
    }
    return false;
}

void ConsolidatorInstance::PublishParameterStateView()
{
    auto view = std::make_shared<ParameterStateView>();
    dspChain_->ReadState(
        StatePath::Instance(state_.GetInstanceId()),
        view->entries);
    std::atomic_store_explicit(
        &publishedParameterState_,
        std::shared_ptr<const ParameterStateView>{std::move(view)},
        std::memory_order_release);
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
