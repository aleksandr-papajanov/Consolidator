#include "Core/Instance/ConsolidatorInstance.h"

#include <algorithm>
#include <cstdint>
#include <exception>
#include <optional>
#include <type_traits>

#include "Core/Coordinator/InstanceCoordinator.h"
#include "Dsp/DspChainBuilder.h"
#include "Dsp/Processors/DspChain.h"

namespace consolidator::core
{

namespace
{

std::optional<dsp::ParameterVariant> ToParameterVariant(const StateValue& value)
{
    return std::visit(
        [](const auto& typedValue) -> std::optional<dsp::ParameterVariant>
        {
            using ValueType = std::decay_t<decltype(typedValue)>;
            if constexpr (std::is_same_v<ValueType, bool> ||
                          std::is_same_v<ValueType, std::int32_t> ||
                          std::is_same_v<ValueType, float>)
            {
                return dsp::ParameterVariant{typedValue};
            }
            return std::nullopt;
        },
        value);
}

} // namespace

ConsolidatorInstance::ConsolidatorInstance()
    : dspChain_(dsp::DspChainBuilder{}.BuildStandardChain())
    , stateStore_(state_)
{
}

ConsolidatorInstance::~ConsolidatorInstance()
{
    if (initialized_)
    {
        InstanceCoordinator::Get().UnregisterInstance(state_.GetInstanceId());
    }
}

void ConsolidatorInstance::Initialize()
{
    if (initialized_)
    {
        return;
    }

    InstanceCoordinator::Get().RegisterInstance(*this);
    PublishInitialRuntimeState();
    initialized_ = true;
}

void ConsolidatorInstance::Process(const double* mainInput,
                                   const double* referenceInput,
                                   double* mainOutput,
                                   double* referenceOutput,
                                   std::size_t frameCount)
{
    DspStateBatch batch;
    if (dspUpdateMailbox_.ConsumeLatest(batch))
    {
        dspChain_->ApplyRuntimeUpdates(batch);
    }
    dspChain_->Process(mainInput, referenceOutput, mainOutput, frameCount, kChannelCount);
    std::copy_n(referenceInput, frameCount * kChannelCount, referenceOutput);
}

InstanceId ConsolidatorInstance::GetInstanceId() const noexcept
{
    return state_.GetInstanceId();
}

dsp::DspChain& ConsolidatorInstance::GetDspChain() noexcept
{
    return *dspChain_;
}

void ConsolidatorInstance::PublishDspUpdates(std::span<const DspUpdate> updates)
{
    for (std::size_t index = 0; index < updates.size(); ++index)
    {
        auto update = updates[index];
        update.revision = ++nextDspRevision_;
        dspUpdateMailbox_.Publish(update);
    }
}

void ConsolidatorInstance::PublishInitialRuntimeState()
{
    StateResponseEntries snapshot;
    stateStore_.ReadState(
        StatePath::Instance(state_.GetInstanceId()),
        snapshot);

    DspStateBatch initialBatch;
    for (std::size_t index = 0; index < snapshot.size; ++index)
    {
        const auto& entry = snapshot.entries[index];
        if (entry.path.field != StateField::DspParameter)
        {
            continue;
        }
        const auto value = ToParameterVariant(entry.value);
        if (!value)
        {
            continue;
        }
        dspUpdateMailbox_.RegisterPath(entry.path);
        if (initialBatch.count == initialBatch.updates.size())
        {
            std::terminate();
        }
        initialBatch.updates[initialBatch.count++] = DspUpdate{
            entry.path,
            *value,
            0};
    }
    PublishDspUpdates(std::span<const DspUpdate>{
        initialBatch.updates.data(),
        initialBatch.count});
}

} // namespace consolidator::core
