#include "PolishAirProcessor.h"

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

double AirHarmonics(double sample) noexcept
{
    const auto square = sample * sample;
    return square * (0.5 + 0.12 * sample) / (1.0 + square);
}

double AirIntegral(double sample) noexcept
{
    const auto square = sample * sample;
    return 0.5 * (sample - std::atan(sample)) + 0.06 * (square - std::log1p(square));
}

double GenerateAir(double sample, double scale, double& previousSample) noexcept
{
    return AntiderivativeAverage(sample, scale, previousSample, AirHarmonics, AirIntegral);
}

} // namespace

void PolishAirProcessor::Prepare(const DspProcessingContext& context) noexcept
{
    amount_.Prepare(context);
    sourceHighPassCoefficient_ = onepole::FilterCoefficient(3000.0, context.sampleRate);
    sourceLowPassCoefficient_ = onepole::FilterCoefficient(6500.0, context.sampleRate);
    outputHighPassCoefficient_ = onepole::FilterCoefficient(10000.0, context.sampleRate);
    shelfCoefficient_ = onepole::FilterCoefficient(10000.0, context.sampleRate);
    topShelfCoefficient_ = onepole::FilterCoefficient(16000.0, context.sampleRate);
    energyCoefficient_ = TimeCoefficient(0.200, context.sampleRate);
    fastEnergyCoefficient_ = TimeCoefficient(0.002, context.sampleRate);
    attenuationAttackCoefficient_ = TimeCoefficient(0.001, context.sampleRate);
    attenuationReleaseCoefficient_ = TimeCoefficient(0.080, context.sampleRate);
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
    const auto selected = SelectChannels(frame.main, frame.plan.channelMode);
    const std::array<double*, 2> audio{ selected.left, selected.right };
    for (std::size_t index = 0; index < frame.main.frameCount; ++index)
    {
        std::array<double, 2> band{};
        std::array<double, 2> shelf{};
        std::array<double, 2> topShelf{};
        auto bandEnergy = 0.0;
        auto mainEnergy = 0.0;
        for (std::size_t channel = 0; channel < channels_.size(); ++channel)
        {
            if (audio[channel] == nullptr)
            {
                continue;
            }
            auto& state = channels_[channel];
            const auto input = audio[channel][index];
            const auto upper = onepole::HighPass(input, sourceHighPassCoefficient_, state.sourceHighPass);
            band[channel] = onepole::LowPass(upper, sourceLowPassCoefficient_, state.sourceLowPass);
            shelf[channel] = onepole::HighPass(input, shelfCoefficient_, state.shelf);
            topShelf[channel] = onepole::HighPass(input, topShelfCoefficient_, state.topShelf);
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
            if (audio[channel] == nullptr)
            {
                continue;
            }
            auto& state = channels_[channel];
            const auto generated = GenerateAir(band[channel], scale, state.previousSample);
            const auto harmonics = onepole::HighPass(
                generated, outputHighPassCoefficient_, state.outputHighPass);
            wet[channel] = 2.6 * amount * amount * attenuation_ * harmonics;
        }
        // Only the added high band is widened; mono has no synthetic side.
        const auto side = audio[0] != nullptr && audio[1] != nullptr ? 0.08 * (wet[0] - wet[1]) : 0.0;
        if (audio[0] != nullptr)
        {
            audio[0][index] += 1.2 * amount * shelf[0] + 0.85 * amount * amount * topShelf[0] + wet[0] + side;
        }
        if (audio[1] != nullptr)
        {
            audio[1][index] += 1.2 * amount * shelf[1] + 0.85 * amount * amount * topShelf[1] + wet[1] - side;
        }
    }
}

} // namespace consolidator::max::dsp
