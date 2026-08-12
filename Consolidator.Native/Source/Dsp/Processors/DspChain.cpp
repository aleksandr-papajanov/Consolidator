#include "Dsp/Processors/DspChain.h"

#include <algorithm>
#include <cassert>
#include <cmath>

#include "Dsp/Processors/Compressor/Compressor.h"
#include "Dsp/Processors/Saturator/Saturator.h"

namespace consolidator::dsp
{

namespace
{

float ToDb(double linearValue) noexcept
{
    return static_cast<float>(
        20.0 * std::log10(std::max(linearValue, 1.0e-12)));
}

} // namespace

void DspChain::AddDevice(std::unique_ptr<DspDevice> device)
{
    assert(devices_.size() < kMaximumDevices);
    devices_.push_back(std::move(device));
}

void DspChain::SetTelemetryProcessors(
    Compressor* compressor,
    Saturator* saturator) noexcept
{
    compressor_ = compressor;
    saturator_ = saturator;
}

void DspChain::SetTelemetryEnabled(bool enabled) noexcept
{
    if (telemetryEnabled_ == enabled)
    {
        return;
    }

    telemetryEnabled_ = enabled;
    for (auto& accumulator : levelAccumulators_)
    {
        accumulator = {};
    }
    for (auto& smoother : levelSmoothers_)
    {
        smoother.Reset();
    }
    for (auto& peakMeter : levelPeakMeters_)
    {
        peakMeter.Reset();
    }
    compressorGainReductionSmoother_.Reset();
    compressorGainReductionPeakMeter_.Reset();
    distortionSmoother_.Reset();
    if (compressor_ != nullptr)
    {
        compressor_->ResetBlockTelemetry();
        compressor_->SetTelemetryEnabled(enabled);
    }
    if (saturator_ != nullptr)
    {
        saturator_->ResetBlockTelemetry();
        saturator_->SetTelemetryEnabled(enabled);
    }
}

void DspChain::Prepare(
    double sampleRate,
    std::size_t channelCount)
{
    for (auto& smoother : levelSmoothers_)
    {
        smoother.SetSampleRate(sampleRate);
    }
    for (auto& peakMeter : levelPeakMeters_)
    {
        peakMeter.SetSampleRate(sampleRate);
    }
    compressorGainReductionSmoother_.SetSampleRate(sampleRate);
    compressorGainReductionPeakMeter_.SetSampleRate(sampleRate);
    distortionSmoother_.SetSampleRate(sampleRate);

    for (const auto& device : devices_)
    {
        device->Prepare(sampleRate, channelCount);
    }
}

void DspChain::ApplyRuntimeUpdates(const core::ParameterUpdateBatch& batch)
{
    std::array<bool, kMaximumDevices> dirtyDevices{};
    for (std::size_t updateIndex = 0; updateIndex < batch.count; ++updateIndex)
    {
        const auto& update = batch.updates[updateIndex];
        for (std::size_t deviceIndex = 0;
             deviceIndex < devices_.size();
             ++deviceIndex)
        {
            if (devices_[deviceIndex]->StageRuntimeUpdate(update.path, update.value))
            {
                dirtyDevices[deviceIndex] = true;
                break;
            }
        }
    }
    for (std::size_t deviceIndex = 0;
         deviceIndex < devices_.size();
         ++deviceIndex)
    {
        if (dirtyDevices[deviceIndex])
        {
            devices_[deviceIndex]->CommitRuntimeUpdates();
        }
    }
}

void DspChain::ApplyRuntimeControlUpdates(
    const core::RuntimeControlBatch& batch)
{
    // Runtime controls only change routing/monitoring flags; parameter runtime is committed.
    for (std::size_t updateIndex = 0; updateIndex < batch.count; ++updateIndex)
    {
        const auto& update = batch.updates[updateIndex];
        const bool appliesToAllEqualizerBanks =
            update.property == core::RuntimeProperty::Active &&
            update.target.GetDeviceId() == DeviceId::Equalizer &&
            update.target.GetDepth() == 0;
        for (const auto& device : devices_)
        {
            const bool applied = update.property == core::RuntimeProperty::Active
                ? device->ApplyProcessingState(update.target, update.value)
                : device->ApplyMonitoringState(update.target, update.value);
            if (applied)
            {
                if (!appliesToAllEqualizerBanks)
                {
                    break;
                }
            }
        }
    }
}

void DspChain::Reset(const core::StatePath& target) noexcept
{
    for (const auto& device : devices_)
    {
        if (device->Reset(target, 0))
        {
            return;
        }
    }
}

void DspChain::Process(
    const double* inputLeft,
    const double* inputRight,
    double* interimLeft,
    double* interimRight,
    double* outputLeft,
    double* outputRight,
    std::size_t frameCount)
{
    const double* sourceLeft = inputLeft;
    const double* sourceRight = inputRight;
    bool hasProcessed = false;
    bool outputContainsResult = false;
    const auto recordLevel = [this](MeterPoint point,
                                    const double* left,
                                    const double* right,
                                    std::size_t count)
    {
        auto& accumulator = levelAccumulators_[ToIndex(point)];
        for (std::size_t frame = 0; frame < count; ++frame)
        {
            const double leftValue = left[frame];
            const double rightValue = right[frame];
            accumulator.sumSquares += leftValue * leftValue + rightValue * rightValue;
            accumulator.peak = std::max(
                accumulator.peak,
                std::max(std::abs(leftValue), std::abs(rightValue)));
            accumulator.sampleCount += 2;
        }
    };

    for (const auto& device : devices_)
    {
        if (!device->IsActive() || device->IsNeutral())
        {
            switch (device->GetDeviceId())
            {
            case DeviceId::MainInputGain:
                if (telemetryEnabled_)
                {
                    recordLevel(
                    MeterPoint::InputGainOutput,
                        sourceLeft, sourceRight, frameCount);
                }
                break;
            case DeviceId::Saturator:
                if (telemetryEnabled_)
                {
                    recordLevel(
                    MeterPoint::SaturatorOutput,
                        sourceLeft, sourceRight, frameCount);
                }
                break;
            case DeviceId::Compressor:
                if (telemetryEnabled_)
                {
                    recordLevel(
                    MeterPoint::CompressorOutput,
                        sourceLeft, sourceRight, frameCount);
                }
                break;
            case DeviceId::MainOutputGain:
                if (telemetryEnabled_)
                {
                    recordLevel(
                    MeterPoint::OutputGainOutput,
                        sourceLeft, sourceRight, frameCount);
                }
                break;
            case DeviceId::Equalizer:
                break;
            }
            continue;
        }

        if (!hasProcessed)
        {
            device->Process(sourceLeft, sourceRight, outputLeft, outputRight, frameCount);
            sourceLeft = outputLeft;
            sourceRight = outputRight;
            hasProcessed = true;
            outputContainsResult = true;
        }
        else
        {
            double* destinationLeft = outputContainsResult ? interimLeft : outputLeft;
            double* destinationRight = outputContainsResult ? interimRight : outputRight;
            device->Process(sourceLeft, sourceRight, destinationLeft, destinationRight, frameCount);
            sourceLeft = destinationLeft;
            sourceRight = destinationRight;
            outputContainsResult = !outputContainsResult;
        }

        const auto output = sourceLeft;
        switch (device->GetDeviceId())
        {
        case DeviceId::MainInputGain:
            if (telemetryEnabled_)
            {
                recordLevel(
                MeterPoint::InputGainOutput,
                    output, sourceRight, frameCount);
            }
            break;
        case DeviceId::Saturator:
            if (telemetryEnabled_)
            {
                recordLevel(
                MeterPoint::SaturatorOutput,
                    output, sourceRight, frameCount);
            }
            break;
        case DeviceId::Compressor:
            if (telemetryEnabled_)
            {
                recordLevel(
                MeterPoint::CompressorOutput,
                    output, sourceRight, frameCount);
            }
            break;
        case DeviceId::MainOutputGain:
            if (telemetryEnabled_)
            {
                recordLevel(
                MeterPoint::OutputGainOutput,
                    output, sourceRight, frameCount);
            }
            break;
        case DeviceId::Equalizer:
            break;
        }
    }

    if (!hasProcessed)
    {
        std::copy_n(inputLeft, frameCount, outputLeft);
        std::copy_n(inputRight, frameCount, outputRight);
    }
    else if (!outputContainsResult)
    {
        std::copy_n(sourceLeft, frameCount, outputLeft);
        std::copy_n(sourceRight, frameCount, outputRight);
    }
}

TelemetrySnapshot DspChain::FinishTelemetryBlock(
    std::size_t frameCount) noexcept
{
    if (!telemetryEnabled_)
    {
        return {};
    }
    TelemetrySnapshot snapshot;
    for (std::size_t index = 0; index < levelAccumulators_.size(); ++index)
    {
        auto& accumulator = levelAccumulators_[index];
        const auto rms = accumulator.sampleCount == 0
            ? 0.0
            : std::sqrt(
                  accumulator.sumSquares /
                  static_cast<double>(accumulator.sampleCount));
        const auto peak = levelPeakMeters_[index].Process(
            static_cast<float>(accumulator.peak), frameCount);
        const auto peakDb = ToDb(peak);
        const auto smoothedRms = levelSmoothers_[index].Process(
            static_cast<float>(rms), frameCount);
        snapshot.levels[index] = {
            ToDb(rms),
            peakDb,
            ToDb(smoothedRms)};
        accumulator = {};
    }

    if (saturator_ != nullptr)
    {
        const auto telemetry = saturator_->GetBlockTelemetry();
        saturator_->ResetBlockTelemetry();
        snapshot.saturator = {
            telemetry.distortionPercent,
            distortionSmoother_.Process(
                telemetry.distortionPercent, frameCount)};
    }
    if (compressor_ != nullptr)
    {
        const auto telemetry = compressor_->GetBlockTelemetry();
        snapshot.compressor = {
            telemetry.gainReductionRmsDb,
            compressorGainReductionPeakMeter_.Process(
                telemetry.gainReductionPeakDb, frameCount),
            compressorGainReductionSmoother_.Process(
                telemetry.gainReductionRmsDb, frameCount)};
        compressor_->ResetBlockTelemetry();
    }
    snapshot.revision = ++telemetryRevision_;
    return snapshot;
}

std::size_t DspChain::GetDeviceCount() const noexcept
{
    return devices_.size();
}

DspDevice* DspChain::GetDevice(std::size_t index) noexcept
{
    return index < devices_.size() ? devices_[index].get() : nullptr;
}

const DspDevice* DspChain::GetDevice(std::size_t index) const noexcept
{
    return index < devices_.size() ? devices_[index].get() : nullptr;
}

} // namespace consolidator::dsp
