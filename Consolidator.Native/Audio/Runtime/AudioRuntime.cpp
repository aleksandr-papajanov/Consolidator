#include "AudioRuntime.h"

#include <algorithm>
#include <atomic>
#include <bit>
#include <cmath>
#include <cstdint>
#include <stdexcept>

#include "SnapshotBinding.h"
#include "../../Transport/DspStateConsumer.h"

namespace consolidator::audio::runtime
{

void AudioRuntime::Prepare(double sampleRate, std::size_t maximumFrameCount)
{
    if (!std::isfinite(sampleRate) || sampleRate <= 0.0)
    {
        throw std::invalid_argument("Audio sample rate must be finite and positive.");
    }
    prepared_ = false;
    context_ = { sampleRate, maximumFrameCount == 0 ? 4096 : maximumFrameCount };
    chain_.Prepare(context_);
    outputMeter_.Prepare(context_);
    if (hasSnapshot_)
    {
        chain_.SetParameters(BindSnapshot(snapshot_));
    }
    Reset();
    prepared_ = true;
}

void AudioRuntime::Reset() noexcept
{
    chain_.Reset();
    outputMeter_.Reset();
}

void AudioRuntime::ProcessBlock(transport::DspStateExchange& exchange,
    double* left, double* right, std::size_t frameCount) noexcept
{
    if (!prepared_ || frameCount == 0 || frameCount > context_.maximumFrameCount ||
        (left == nullptr && right == nullptr))
    {
        return;
    }
    if (transport::ConsumePublishedDspState(exchange, consumerIndex_, snapshot_))
    {
        chain_.SetParameters(BindSnapshot(snapshot_));
        if (!hasSnapshot_)
        {
            // The first published state seeds histories/ramps; later states only change targets.
            chain_.Reset();
            hasSnapshot_ = true;
        }
    }
    if (!hasSnapshot_)
    {
        return;
    }
    AudioBuffer audio{ left, right, frameCount };
    if (snapshot_.audible == 0)
    {
        if (left != nullptr)
        {
            std::fill_n(left, frameCount, 0.0);
        }
        if (right != nullptr)
        {
            std::fill_n(right, frameCount, 0.0);
        }
    }
    const auto processed = snapshot_.audible != 0 && snapshot_.instanceBypass == 0;
    if (processed)
    {
        chain_.Process(audio, ChannelMode::Stereo);
    }
    outputMeter_.Process(audio, ChannelMode::Stereo);
    PublishTelemetry(exchange.telemetry, processed);
}

void AudioRuntime::PublishTelemetry(transport::DspTelemetryExchange& exchange, bool processed) noexcept
{
    const auto values = chain_.Telemetry();
    using Values = transport::DspTelemetryValues;
    auto present = static_cast<std::uint32_t>(Values::OutputLevel);
    if (snapshot_.inputGainBypass == 0)
    {
        if (processed)
        {
            present |= static_cast<std::uint32_t>(Values::InputRms);
        }
        if (snapshot_.inputLeveler != 0)
        {
            present |= static_cast<std::uint32_t>(Values::InputLevel) |
                static_cast<std::uint32_t>(Values::InputLevelFast);
        }
    }
    if (snapshot_.outputGainBypass == 0)
    {
        if (processed)
        {
            present |= static_cast<std::uint32_t>(Values::OutputRms);
        }
        if (snapshot_.outputLeveler != 0)
        {
            present |= static_cast<std::uint32_t>(Values::OutputGain) |
                static_cast<std::uint32_t>(Values::OutputGainFast);
        }
    }
    present |= static_cast<std::uint32_t>(Values::CompressorReduction) |
        static_cast<std::uint32_t>(Values::LimiterReduction);
    const auto bits = [](double value)
    {
        return std::bit_cast<std::uint32_t>(static_cast<float>(value));
    };
    std::atomic_ref sequence{ exchange.sequence };
    sequence.fetch_add(1, std::memory_order_acq_rel);
    exchange.valuesPresent = present;
    exchange.inputLevelBits = bits(values.input.levelDb);
    exchange.inputLevelFastBits = bits(values.input.fastCorrectionDb);
    exchange.outputLevelBits = bits(outputMeter_.LevelDb());
    exchange.inputRmsBits = bits(values.input.rmsDb);
    exchange.outputGainBits = bits(values.output.levelDb);
    exchange.outputGainFastBits = bits(values.output.fastCorrectionDb);
    exchange.outputRmsBits = bits(values.output.rmsDb);
    exchange.compressorReductionBits = bits(processed ? values.compressorReductionDb : 0.0);
    exchange.limiterReductionBits = bits(processed ? values.limiterReductionDb : 0.0);
    sequence.fetch_add(1, std::memory_order_release);
}

} // namespace consolidator::audio::runtime
