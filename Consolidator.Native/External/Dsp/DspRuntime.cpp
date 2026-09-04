#include "DspRuntime.h"

#include <atomic>
#include <bit>
#include <cmath>

namespace consolidator::max::dsp
{

void DspRuntime::Prepare(
    double sampleRate,
    std::size_t maximumFrameCount)
{
    graph_.Prepare(sampleRate, maximumFrameCount);
    constexpr double outputRmsTimeSeconds = 0.3;
    outputRmsTimeSamples_ = sampleRate > 0.0
        ? sampleRate * outputRmsTimeSeconds
        : 1.0;
}

void DspRuntime::Reset() noexcept
{
    graph_.Reset();
    outputMeanSquare_ = 0.0;
}

void DspRuntime::ProcessBlock(
    DspStateExchange& exchange,
    double* mainLeft,
    double* mainRight,
    std::size_t frameCount) noexcept
{
    if (ConsumePublishedDspState(
            exchange,
            consumerIndex_,
            snapshot_))
    {
        graph_.SetParameters(snapshot_);
    }

    graph_.ProcessBlock(
        mainLeft,
        mainRight,
        frameCount);

    if (frameCount != 0 && (mainLeft != nullptr || mainRight != nullptr))
    {
        double sum = 0.0;
        const auto channelCount = mainLeft != nullptr && mainRight != nullptr
            ? 2.0
            : 1.0;
        for (std::size_t index = 0; index < frameCount; ++index)
        {
            const auto left = mainLeft != nullptr ? mainLeft[index] : 0.0;
            const auto right = mainRight != nullptr ? mainRight[index] : 0.0;
            sum += (left * left + right * right) / channelCount;
        }

        const auto blockMeanSquare = sum / static_cast<double>(frameCount);
        const auto coefficient = 1.0 - std::exp(
            -static_cast<double>(frameCount) / outputRmsTimeSamples_);
        outputMeanSquare_ += coefficient * (blockMeanSquare - outputMeanSquare_);
        if (!std::isfinite(outputMeanSquare_) || outputMeanSquare_ < 0.0)
        {
            outputMeanSquare_ = 0.0;
        }
    }

    constexpr double minimumMeanSquare = 1.0e-12;
    const auto outputLevelDb = static_cast<float>(
        10.0 * std::log10(
            outputMeanSquare_ > minimumMeanSquare
                ? outputMeanSquare_
                : minimumMeanSquare));

    auto& telemetry = exchange.telemetry;
    std::atomic_ref sequence{ telemetry.sequence };
    sequence.fetch_add(1, std::memory_order_acq_rel);
    telemetry.valuesPresent = static_cast<std::uint32_t>(
        DspTelemetryValues::OutputLevel);
    if (graph_.HasInputLevelTelemetry())
    {
        telemetry.valuesPresent |= static_cast<std::uint32_t>(
            DspTelemetryValues::InputLevel) |
            static_cast<std::uint32_t>(DspTelemetryValues::InputLevelFast);
    }
    telemetry.inputLevelBits = std::bit_cast<std::uint32_t>(
        graph_.InputLevelTelemetry());
    telemetry.inputLevelFastBits = std::bit_cast<std::uint32_t>(
        graph_.FastInputLevelTelemetry());
    telemetry.outputLevelBits = std::bit_cast<std::uint32_t>(outputLevelDb);
    sequence.fetch_add(1, std::memory_order_release);
}

} // namespace consolidator::max::dsp
