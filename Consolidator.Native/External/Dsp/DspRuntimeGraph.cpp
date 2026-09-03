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
    equalizerDetector_.Prepare(context_);
    leveler_.Prepare(context_);
    saturator_.Prepare(context_);
    compressor_.Prepare(context_);
    equalizer_.Prepare(context_);
    compressorParallel_.Prepare(context_);
    polishParallel_.Prepare(context_);
    polish_.Prepare(context_);
    output_.Prepare(context_);
    limiter_.Prepare(context_);
}

void DspRuntimeGraph::Reset() noexcept
{
    input_.Reset();
    equalizerDetector_.Reset();
    leveler_.Reset();
    saturator_.Reset();
    compressor_.Reset();
    equalizer_.Reset();
    compressorParallel_.Reset();
    polishParallel_.Reset();
    polish_.Reset();
    output_.Reset();
    limiter_.Reset();
}

void DspRuntimeGraph::SetParameters(
    const DspSnapshot& snapshot) noexcept
{
    snapshot_ = snapshot;
    policy_ = BuildExecutionPolicy(snapshot_, plan_);
    compressorParallel_.SetEnabled(policy_.compressorParallel);
    polishParallel_.SetEnabled(policy_.polishActive);
    input_.SetParameters(snapshot);
    equalizerDetector_.SetParameters(snapshot);
    leveler_.SetParameters(snapshot);
    saturator_.SetParameters(snapshot);
    compressor_.SetParameters(snapshot);
    equalizer_.SetParameters(snapshot);
    polish_.SetParameters(snapshot);
    output_.SetParameters(snapshot);
    limiter_.SetParameters(snapshot);
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
    const double* sidechainLeft,
    const double* sidechainRight,
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
    const DspSidechainBuffer externalSidechain{ sidechainLeft, sidechainRight };
    DspProcessingFrame frame{
        main,
        externalSidechain,
        externalSidechain,
        plan_,
        policy_ };

    if (policy_.inputActive)
    {
        input_.Process(frame);
    }
    if (policy_.equalizerActive)
    {
        equalizerDetector_.Process(frame);
        frame.detectorSidechain = frame.externalSidechain;
    }
    if (policy_.levelerEnabled)
    {
        leveler_.Process(frame);
    }
    if (policy_.saturatorActive)
    {
        saturator_.Process(frame);
    }
    if (policy_.compressorActive)
    {
        compressor_.Process(frame);
    }
    if (policy_.equalizerActive)
    {
        equalizer_.Process(frame);
    }
    if (policy_.compressorParallel)
    {
        compressorParallel_.Process(frame);
    }
    if (policy_.polishActive)
    {
        polish_.Process(frame);
        polishParallel_.Process(frame);
    }
    if (policy_.outputActive)
    {
        output_.Process(frame);
    }
    if (policy_.limiterEnabled)
    {
        limiter_.Process(frame);
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

} // namespace consolidator::max::dsp
