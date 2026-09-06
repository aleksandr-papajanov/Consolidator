#include "PolishThickProcessor.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

#include "DspAntiderivative.h"
#include "DspMath.h"
#include "DspOnePole.h"
#include "DspProcessingPrimitives.h"

namespace consolidator::max::dsp
{

namespace
{

// For a unit sine, these bases isolate H2, H3 and H4 (plus DC in
// the even bases). Outside +/-1 the slope decays exponentially to zero.
double BassShape(double sample, double third, double fourth) noexcept
{
    const auto x = std::clamp(sample, -1.0, 1.0);
    const auto square = x * x;
    const auto value = 0.9 * square + third * x * (square - 0.75) + fourth * square * (square - 1.0);
    if (sample == x)
    {
        return value;
    }
    const auto slope = 1.8 * x + third * (3.0 * square - 0.75) + fourth * x * (4.0 * square - 2.0);
    const auto distance = std::abs(sample - x);
    return value + x * slope * 0.2 * (-std::expm1(-distance / 0.2));
}

double BassIntegral(double sample, double third, double fourth) noexcept
{
    const auto x = std::clamp(sample, -1.0, 1.0);
    const auto square = x * x;
    const auto integral = 0.3 * x * square + third * (0.25 * square * square - 0.375 * square)
        + fourth * x * square * (0.2 * square - 1.0 / 3.0);
    if (sample == x)
    {
        return integral;
    }
    const auto distance = std::abs(sample - x);
    const auto slope = 1.8 * x + third * (3.0 * square - 0.75) + fourth * x * (4.0 * square - 2.0);
    // Integral of the bounded extension, continuous with the polynomial.
    return integral + x * distance * BassShape(x, third, fourth)
        + slope * 0.2 * (distance + 0.2 * std::expm1(-distance / 0.2));
}

double GenerateBass(double sample, double scale, double amount, double& previousSample) noexcept
{
    const auto third = 0.35 + 0.35 * amount;
    const auto fourth = 0.1 + 0.15 * amount;
    return AntiderivativeAverage(sample, scale, previousSample,
        [third, fourth](double value) noexcept
        {
            return BassShape(value, third, fourth);
        },
        [third, fourth](double value) noexcept
        {
            return BassIntegral(value, third, fourth);
        });
}

} // namespace

void PolishThickProcessor::Prepare(const DspProcessingContext& context) noexcept
{
    amount_.Prepare(context);
    rumbleCoefficient_ = onepole::FilterCoefficient(20.0, context.sampleRate);
    sourceCoefficient_ = onepole::FilterCoefficient(50.0, context.sampleRate);
    highPassCoefficient_ = onepole::FilterCoefficient(50.0, context.sampleRate);
    lowPassCoefficient_ = onepole::FilterCoefficient(400.0, context.sampleRate);
    energyCoefficient_ = TimeCoefficient(0.200, context.sampleRate);
    fastEnergyCoefficient_ = TimeCoefficient(0.020, context.sampleRate);
    peakAttackCoefficient_ = TimeCoefficient(0.001, context.sampleRate);
    peakReleaseCoefficient_ = TimeCoefficient(0.120, context.sampleRate);
    gainAttackCoefficient_ = TimeCoefficient(0.001, context.sampleRate);
    gainReleaseCoefficient_ = TimeCoefficient(0.080, context.sampleRate);
}

void PolishThickProcessor::Reset() noexcept
{
    amount_.Reset();
    channels_ = {};
    energy_ = 0.0;
    energyWeight_ = 0.0;
    wetEnergy_ = 0.0;
    fastWetEnergy_ = 0.0;
    fastEnergyWeight_ = 0.0;
    harmonicGain_ = 1.0;
    sourcePeak_ = 0.0;
}

void PolishThickProcessor::SetParameters(const DspSnapshot& snapshot) noexcept
{
    amount_.SetTarget(snapshot.polishThick);
}

void PolishThickProcessor::Process(DspProcessingFrame& frame) noexcept
{
    const auto selected = SelectChannels(frame.main, frame.plan.channelMode);
    const std::array<double*, 2> audio{ selected.left, selected.right };
    for (std::size_t index = 0; index < frame.main.frameCount; ++index)
    {
        std::array<double, 2> bass{};
        auto energy = 0.0;
        for (std::size_t channel = 0; channel < channels_.size(); ++channel)
        {
            if (audio[channel] != nullptr)
            {
                auto& state = channels_[channel];
                const auto source = onepole::HighPass(audio[channel][index], rumbleCoefficient_, state.rumble);
                bass[channel] = onepole::LowPass(source, sourceCoefficient_, state.source);
                energy = std::max(energy, bass[channel] * bass[channel]);
            }
        }
        energy_ += energyCoefficient_ * (energy - energy_);
        energyWeight_ += energyCoefficient_ * (1.0 - energyWeight_);
        const auto meanEnergy = energy_ / std::max(energyWeight_, 1.0e-12);
        const auto peak = std::sqrt(energy);
        const auto peakCoefficient = peak > sourcePeak_ ? peakAttackCoefficient_ : peakReleaseCoefficient_;
        sourcePeak_ += peakCoefficient * (peak - sourcePeak_);
        // Catch new attacks without waiting for the long-term RMS reference.
        const auto scale = std::max(sourcePeak_, std::sqrt(std::max(2.0 * meanEnergy, 1.0e-10)));
        const auto amount = std::clamp(amount_.NextValue(), 0.0, 1.0);
        std::array<double, 2> wet{};
        auto wetEnergy = 0.0;
        for (std::size_t channel = 0; channel < channels_.size(); ++channel)
        {
            if (audio[channel] == nullptr)
            {
                continue;
            }
            auto& state = channels_[channel];
            const auto generated = GenerateBass(bass[channel], scale, amount, state.previousSample);
            const auto upperBass = onepole::HighPass(generated, highPassCoefficient_, state.highPass);
            wet[channel] = onepole::LowPass(upperBass, lowPassCoefficient_, state.lowPass);
            wetEnergy = std::max(wetEnergy, wet[channel] * wet[channel]);
        }
        wetEnergy_ += energyCoefficient_ * (wetEnergy - wetEnergy_);
        fastWetEnergy_ += fastEnergyCoefficient_ * (wetEnergy - fastWetEnergy_);
        fastEnergyWeight_ += fastEnergyCoefficient_ * (1.0 - fastEnergyWeight_);
        const auto meanWet = wetEnergy_ / std::max(energyWeight_, 1.0e-12);
        const auto fastWet = fastWetEnergy_ / std::max(fastEnergyWeight_, 1.0e-12);
        auto desiredGain = 1.0;
        if (meanEnergy > 1.0e-10 && fastWet > 1.0e-10)
        {
            // Lift the body/tail, but reduce gain on wet bursts instead of
            // retaining the boosted tail gain through the next attack.
            const auto calibration = std::clamp(std::sqrt(meanEnergy / std::max(meanWet, 1.0e-10)), 0.5, 3.0);
            const auto upward = std::clamp(std::sqrt(std::sqrt(meanWet / fastWet)), 1.0, 1.5);
            const auto burstReduction = std::sqrt(std::min(1.0, meanWet / fastWet));
            desiredGain = std::clamp(calibration * upward * burstReduction, 0.25, 4.0);
        }
        const auto gainCoefficient = desiredGain < harmonicGain_ ? gainAttackCoefficient_ : gainReleaseCoefficient_;
        harmonicGain_ += gainCoefficient * (desiredGain - harmonicGain_);
        const auto intensity = amount * (0.46875 + 1.03125 * amount);
        for (std::size_t channel = 0; channel < channels_.size(); ++channel)
        {
            if (audio[channel] != nullptr)
            {
                audio[channel][index] += intensity * harmonicGain_ * wet[channel];
            }
        }
    }
}

} // namespace consolidator::max::dsp
