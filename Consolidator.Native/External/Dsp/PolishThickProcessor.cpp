#include "DspProcessors.h"

#include <algorithm>
#include <cmath>

#include "DspExciterPrimitives.h"

namespace consolidator::max::dsp
{

void PolishThickProcessor::Prepare(const DspProcessingContext& context) noexcept
{
    amount_.Prepare(context);
    sourceCoefficient_ = exciter::FilterCoefficient(120.0, context.sampleRate);
    highPassCoefficient_ = exciter::FilterCoefficient(100.0, context.sampleRate);
    lowPassCoefficient_ = exciter::FilterCoefficient(700.0, context.sampleRate);
    energyCoefficient_ = exciter::TimeCoefficient(0.200, context.sampleRate);
}

void PolishThickProcessor::Reset() noexcept
{
    amount_.Reset();
    channels_ = {};
    energy_ = 0.0;
    energyWeight_ = 0.0;
}

void PolishThickProcessor::SetParameters(const DspSnapshot& snapshot) noexcept
{
    amount_.SetTarget(snapshot.polishThick);
}

void PolishThickProcessor::Process(DspProcessingFrame& frame) noexcept
{
    const std::array<bool, 2> selected{
        frame.plan.channelMode != DspChannelMode::Right && frame.main.left != nullptr,
        frame.plan.channelMode != DspChannelMode::Left && frame.main.right != nullptr
    };
    const std::array<double*, 2> audio{ frame.main.left, frame.main.right };
    for (std::size_t index = 0; index < frame.main.frameCount; ++index)
    {
        std::array<double, 2> bass{};
        auto energy = 0.0;
        for (std::size_t channel = 0; channel < channels_.size(); ++channel)
        {
            if (selected[channel])
            {
                bass[channel] = exciter::LowPass(
                    audio[channel][index], sourceCoefficient_, channels_[channel].source);
                energy = std::max(energy, bass[channel] * bass[channel]);
            }
        }
        energy_ += energyCoefficient_ * (energy - energy_);
        energyWeight_ += energyCoefficient_ * (1.0 - energyWeight_);
        const auto meanEnergy = energy_ / std::max(energyWeight_, 1.0e-12);
        const auto scale = std::sqrt(std::max(2.0 * meanEnergy, 1.0e-10));
        const auto amount = std::clamp(amount_.NextValue(), 0.0, 1.0);
        for (std::size_t channel = 0; channel < channels_.size(); ++channel)
        {
            if (!selected[channel])
            {
                continue;
            }
            auto& state = channels_[channel];
            const auto generated = exciter::Generate(bass[channel], scale, state.previousSample);
            const auto upperBass = exciter::HighPass(generated, highPassCoefficient_, state.highPass);
            const auto wet = exciter::LowPass(upperBass, lowPassCoefficient_, state.lowPass);
            audio[channel][index] += 6.0 * amount * wet;
        }
    }
}

} // namespace consolidator::max::dsp
