#include "DspRuntimeGraph.h"

#include <cmath>

namespace consolidator::max::dsp
{

void DspRuntimeGraph::Prepare(
    double sampleRate,
    std::size_t maximumFrameCount)
{
    context_.sampleRate = std::isfinite(sampleRate) && sampleRate > 0.0
        ? sampleRate
        : 48000.0;
    context_.maximumFrameCount = maximumFrameCount == 0
        ? DspProcessingContext::kDefaultMaximumFrameCount
        : maximumFrameCount;

    input_.Prepare(context_);
    saturator_.Prepare(context_);
    saturatorStage_.Prepare(context_);
    compressor_.Prepare(context_);
    compressorParallel_.Prepare(context_);
    equalizer_.Prepare(context_);
    polish_.Prepare(context_);
    polishStage_.Prepare(context_);
    output_.Prepare(context_);
}

void DspRuntimeGraph::Reset() noexcept
{
    input_.Reset();
    saturator_.Reset();
    saturatorStage_.Reset();
    compressor_.Reset();
    compressorParallel_.Reset();
    equalizer_.Reset();
    polish_.Reset();
    polishStage_.Reset();
    output_.Reset();
}

void DspRuntimeGraph::SetParameters(
    const DspSnapshot& snapshot) noexcept
{
    snapshot_ = snapshot;
    policy_ = BuildExecutionPolicy(snapshot_, plan_);
    input_.SetParameters(snapshot);
    saturator_.SetParameters(snapshot);
    compressor_.SetParameters(snapshot);
    equalizer_.SetParameters(snapshot);
    polish_.SetParameters(snapshot);
    output_.SetParameters(snapshot);

    saturatorStage_.SetWetTarget(policy_.saturatorActive ? 1.0 : 0.0);
    compressorParallel_.SetWetTarget(policy_.compressorActive ?
        (policy_.compressorParallel ? 0.5 : 1.0) : 0.0);
    polishStage_.SetWetTarget(policy_.polishActive ? 1.0 : 0.0);
}

void DspRuntimeGraph::SetProcessingPlan(
    const DspProcessingPlan& plan) noexcept
{
    if (!plan.frequencyBands.IsValid(context_.sampleRate))
    {
        return;
    }

    plan_ = plan;
    policy_ = BuildExecutionPolicy(snapshot_, plan_);
}

void DspRuntimeGraph::ProcessBlock(
    double* mainLeft,
    double* mainRight,
    std::size_t frameCount) noexcept
{
    const auto leftRequired = plan_.channelMode != DspChannelMode::Right;
    const auto rightRequired = plan_.channelMode != DspChannelMode::Left;
    if ((leftRequired && mainLeft == nullptr) ||
        (rightRequired && mainRight == nullptr) ||
        frameCount > context_.maximumFrameCount)
    {
        return;
    }

    if (!policy_.audible)
    {
        for (std::size_t index = 0; index < frameCount; ++index)
        {
            if (mainLeft != nullptr)
            {
                mainLeft[index] = 0.0;
            }
            if (mainRight != nullptr)
            {
                mainRight[index] = 0.0;
            }
        }
        return;
    }

    if (policy_.instanceBypassed)
    {
        return;
    }

    DspAudioBuffer main{ mainLeft, mainRight, frameCount };
    DspProcessingFrame frame{
        main,
        {},
        plan_,
        policy_ };

    if (!policy_.inputBypassed &&
        (policy_.inputActive || input_.RequiresProcessing()))
    {
        input_.Process(frame);
    }
    saturatorStage_.Process(frame, saturator_);
    compressorParallel_.Process(frame, compressor_);
    if (policy_.equalizerActive || equalizer_.RequiresProcessing())
    {
        equalizer_.Process(frame);
    }
    polishStage_.Process(frame, polish_);
    if (!policy_.outputBypassed &&
        (policy_.outputActive ||
            policy_.limiterEnabled ||
            output_.RequiresProcessing()))
    {
        output_.Process(frame);
    }
}

const DspProcessingContext& DspRuntimeGraph::Context() const noexcept
{
    return context_;
}

const DspProcessingPlan& DspRuntimeGraph::ProcessingPlan() const noexcept
{
    return plan_;
}

bool DspRuntimeGraph::HasInputLevelTelemetry() const noexcept
{
    return input_.IsLevelerEnabled();
}

float DspRuntimeGraph::InputLevelTelemetry() const noexcept
{
    return input_.SlowInputLevelDb();
}

float DspRuntimeGraph::FastInputLevelTelemetry() const noexcept
{
    return input_.FastInputLevelDb();
}

} // namespace consolidator::max::dsp
