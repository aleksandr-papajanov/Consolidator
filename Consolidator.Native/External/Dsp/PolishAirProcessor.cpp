#include "DspProcessors.h"

#include <algorithm>
#include <cmath>

#include "DspExciterPrimitives.h"

namespace consolidator::max::dsp
{

void PolishAirProcessor::Prepare(const DspProcessingContext& context) noexcept
{
    amount_.Prepare(context);
    sourceHighPassCoefficient_ = exciter::FilterCoefficient(3000.0, context.sampleRate);
    sourceLowPassCoefficient_ = exciter::FilterCoefficient(8000.0, context.sampleRate);
    outputHighPassCoefficient_ = exciter::FilterCoefficient(7000.0, context.sampleRate);
    shelfCoefficient_ = exciter::FilterCoefficient(9000.0, context.sampleRate);
    energyCoefficient_ = exciter::TimeCoefficient(0.200, context.sampleRate);
    fastEnergyCoefficient_ = exciter::TimeCoefficient(0.002, context.sampleRate);
    attenuationAttackCoefficient_ = exciter::TimeCoefficient(0.001, context.sampleRate);
    attenuationReleaseCoefficient_ = exciter::TimeCoefficient(0.080, context.sampleRate);
}

void PolishAirProcessor::Reset() noexcept
{
    amount_.Reset();
    channels_ = {};
    bandEnergy_ = 0.0;
    mainEnergy_ = 0.0;
    fastBandEnergy_ = 0.0;
    energyWeight_ = 0.0;
    fastEnergyWeight_ = 0.0;
    attenuation_ = 1.0;
}

void PolishAirProcessor::SetParameters(const DspSnapshot& snapshot) noexcept
{
    amount_.SetTarget(snapshot.polishAir);
}

void PolishAirProcessor::Process(DspProcessingFrame& frame) noexcept
{
    const std::array<bool, 2> selected{
        frame.plan.channelMode != DspChannelMode::Right && frame.main.left != nullptr,
        frame.plan.channelMode != DspChannelMode::Left && frame.main.right != nullptr
    };
    const std::array<double*, 2> audio{ frame.main.left, frame.main.right };
    for (std::size_t index = 0; index < frame.main.frameCount; ++index)
    {
        std::array<double, 2> band{};
        std::array<double, 2> shelf{};
        auto bandEnergy = 0.0;
        auto mainEnergy = 0.0;
        for (std::size_t channel = 0; channel < channels_.size(); ++channel)
        {
            if (!selected[channel])
            {
                continue;
            }
            auto& state = channels_[channel];
            const auto input = audio[channel][index];
            const auto upper = exciter::HighPass(input, sourceHighPassCoefficient_, state.sourceHighPass);
            band[channel] = exciter::LowPass(upper, sourceLowPassCoefficient_, state.sourceLowPass);
            shelf[channel] = exciter::HighPass(input, shelfCoefficient_, state.shelf);
            bandEnergy = std::max(bandEnergy, band[channel] * band[channel]);
            mainEnergy = std::max(mainEnergy, input * input);
        }
        bandEnergy_ += energyCoefficient_ * (bandEnergy - bandEnergy_);
        mainEnergy_ += energyCoefficient_ * (mainEnergy - mainEnergy_);
        fastBandEnergy_ += fastEnergyCoefficient_ * (bandEnergy - fastBandEnergy_);
        energyWeight_ += energyCoefficient_ * (1.0 - energyWeight_);
        fastEnergyWeight_ += fastEnergyCoefficient_ * (1.0 - fastEnergyWeight_);
        const auto meanBandEnergy = bandEnergy_ / std::max(energyWeight_, 1.0e-12);
        const auto meanMainEnergy = mainEnergy_ / std::max(energyWeight_, 1.0e-12);
        const auto fastMeanEnergy = fastBandEnergy_ / std::max(fastEnergyWeight_, 1.0e-12);
        // Both windows are normalized during startup: their different time
        // constants must not turn the first sample into a false harshness burst.
        const auto spectralDensity = meanBandEnergy / std::max(meanMainEnergy, 1.0e-10);
        const auto densityExcess = std::max(0.0, spectralDensity - 0.15);
        const auto burst = std::max(0.0, fastMeanEnergy / std::max(meanBandEnergy, 1.0e-10) - 3.0);
        const auto desiredAttenuation = 1.0 / (1.0 + 8.0 * densityExcess + 0.75 * burst);
        const auto attenuationCoefficient = desiredAttenuation < attenuation_
            ? attenuationAttackCoefficient_ : attenuationReleaseCoefficient_;
        attenuation_ += attenuationCoefficient * (desiredAttenuation - attenuation_);

        const auto scale = std::sqrt(std::max(2.0 * meanBandEnergy, 1.0e-10));
        const auto amount = std::clamp(amount_.NextValue(), 0.0, 1.0);
        std::array<double, 2> wet{};
        for (std::size_t channel = 0; channel < channels_.size(); ++channel)
        {
            if (!selected[channel])
            {
                continue;
            }
            auto& state = channels_[channel];
            const auto generated = exciter::Generate(band[channel], scale, state.previousSample);
            const auto harmonics = exciter::HighPass(
                generated, outputHighPassCoefficient_, state.outputHighPass);
            // Dry plus a single-pole high-pass produces a monotonic air shelf.
            wet[channel] = attenuation_ * (4.0 * harmonics + 0.60 * shelf[channel]);
        }
        // Only the added high band is widened; mono has no synthetic side.
        const auto side = selected[0] && selected[1] ? 0.08 * (wet[0] - wet[1]) : 0.0;
        if (selected[0])
        {
            audio[0][index] += amount * (wet[0] + side);
        }
        if (selected[1])
        {
            audio[1][index] += amount * (wet[1] - side);
        }
    }
}

} // namespace consolidator::max::dsp
