#include "Core/Instance/ConsolidatorInstance.h"

#include <algorithm>
#include <cstdint>
#include <exception>
#include <optional>
#include <type_traits>
#include <vector>

#include "Core/Coordinator/InstanceCoordinator.h"
#include "Core/Routing/ProcessingStateResolver.h"
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
    , stateStore_()
{
}

ConsolidatorInstance::~ConsolidatorInstance()
{
    if (initialized_)
    {
        ShutdownResponseNotifier();
        InstanceCoordinator::Get().UnregisterInstance(stateStore_.GetInstanceId());
        InstanceCoordinator::Get().RefreshAudibility();
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
    InstanceCoordinator::Get().RefreshAudibility();
}

void ConsolidatorInstance::Prepare(double sampleRate)
{
    dspChain_->Prepare(sampleRate, kChannelCount);
}

void ConsolidatorInstance::Process(const double* mainInputLeft,
                                   const double* mainInputRight,
                                   const double* referenceInputLeft,
                                   const double* referenceInputRight,
                                   double* mainOutputLeft,
                                   double* mainOutputRight,
                                   double* referenceOutputLeft,
                                   double* referenceOutputRight,
                                   std::size_t frameCount)
{
    // Block-start invariant: state snapshots, controls, and ordered commands
    // are applied before processing the audio block.
    ConsumeParameterUpdates();
    ConsumeRuntimeUpdates();
    ProcessRealtimeCommands();

    dspChain_->Process(mainInputLeft, mainInputRight,
                       referenceOutputLeft, referenceOutputRight,
                       mainOutputLeft, mainOutputRight,
                       frameCount);
    std::copy_n(referenceInputLeft, frameCount, referenceOutputLeft);
    std::copy_n(referenceInputRight, frameCount, referenceOutputRight);

    ApplyOutputGate(mainOutputLeft, frameCount);
    ApplyOutputGate(mainOutputRight, frameCount);
}

void ConsolidatorInstance::ConsumeParameterUpdates()
{
    ParameterUpdateBatch batch;
    if (runtimeUpdateMailbox_.ConsumeLatest(batch))
    {
        dspChain_->ApplyRuntimeUpdates(batch);
    }
}

void ConsolidatorInstance::ConsumeRuntimeUpdates()
{
    RuntimeControlBatch controlBatch;
    if (!runtimeUpdateMailbox_.ConsumeControlLatest(controlBatch))
    {
        return;
    }

    RuntimeControlBatch deviceControlBatch;
    deviceControlBatch.revision = controlBatch.revision;
    for (std::size_t index = 0; index < controlBatch.count; ++index)
    {
        const auto& update = controlBatch.updates[index];
        if (update.property == RuntimeProperty::OutputEnabled &&
            update.target.instanceId == GetInstanceId())
        {
            outputEnabled_ = update.value;
            continue;
        }
        deviceControlBatch.updates[deviceControlBatch.count++] = update;
    }
    if (deviceControlBatch.count != 0)
    {
        dspChain_->ApplyRuntimeControlUpdates(deviceControlBatch);
    }
}

void ConsolidatorInstance::ProcessRealtimeCommands()
{
    while (const auto realtimeCommand = realtimeCommandQueue_.TryDequeue())
    {
        std::visit(
            [this](const auto& command)
            {
                using CommandType = std::decay_t<decltype(command)>;
                if constexpr (std::is_same_v<CommandType, ResetRuntimeCommand>)
                {
                    dspChain_->Reset(command.target);
                }
            },
            *realtimeCommand);
    }
}

void ConsolidatorInstance::ApplyOutputGate(
    double* mainOutput,
    std::size_t sampleCount) const
{
    if (!outputEnabled_)
    {
        std::fill_n(mainOutput, sampleCount, 0.0);
    }
}

InstanceId ConsolidatorInstance::GetInstanceId() const noexcept
{
    return stateStore_.GetInstanceId();
}

dsp::DspChain& ConsolidatorInstance::GetDspChain() noexcept
{
    return *dspChain_;
}

void ConsolidatorInstance::PublishInitialRuntimeState()
{
    std::vector<StateEntry> parameters;
    stateStore_.ReadRuntimeParameters(parameters);

    ParameterUpdateBatch initialBatch;
    for (const auto& entry : parameters)
    {
        const auto value = ToParameterVariant(entry.value);
        if (!value)
        {
            continue;
        }
        runtimeUpdateMailbox_.RegisterPath(entry.path);
        if (initialBatch.count == initialBatch.updates.size())
        {
            std::terminate();
        }
        initialBatch.updates[initialBatch.count++] = ParameterUpdate{
            entry.path,
            *value,
            0};
    }

    RuntimeResolution resolution;
    ProcessingStateResolver{}.Resolve(
        stateStore_.GetInstanceId(),
        stateStore_,
        resolution);
    for (const auto& update : resolution.controls)
    {
        runtimeUpdateMailbox_.RegisterControlPath(update.target, update.property);
    }
    runtimeUpdateMailbox_.RegisterControlPath(
        StatePath::Instance(stateStore_.GetInstanceId()),
        RuntimeProperty::OutputEnabled);
    EnqueueParameterUpdates(std::span<const ParameterUpdate>{
        initialBatch.updates.data(),
        initialBatch.count});
    EnqueueRuntimeUpdates(std::span<const RuntimeControlUpdate>{
        resolution.controls.data(),
        resolution.controls.size()});
}

} // namespace consolidator::core
