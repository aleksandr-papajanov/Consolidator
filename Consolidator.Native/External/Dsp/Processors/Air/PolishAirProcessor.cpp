#include "PolishAirProcessor.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <numbers>

#include "../../DspMath.h"
#include "../../DspOnePole.h"
#include "../../DspProcessingPrimitives.h"

namespace consolidator::max::dsp::processors
{

void PolishAirProcessor::Prepare(const DspProcessingContext& context) noexcept
{
    amount_.Prepare(context);
    PrepareFilters(context.sampleRate);
    peakReleaseCoefficient_ = TimeCoefficient(2.0, context.sampleRate);
    peakHoldSamples_ = static_cast<std::size_t>(context.sampleRate);
}

void PolishAirProcessor::Reset() noexcept
{
    amount_.Reset();
    channels_ = {};
    inputPeak_ = 0.0;
    peakHoldRemaining_ = 0;
    linearLevelGain_ = 1.0;
}

void PolishAirProcessor::SetParameters(const DspSnapshot& snapshot) noexcept
{
    const auto amount = std::clamp(
        static_cast<double>(snapshot.polishAir),
        0.0,
        1.0);
    amount_.SetTarget(amount);
}

void PolishAirProcessor::Process(DspProcessingFrame& frame) noexcept
{
    const auto selected = SelectChannels(frame.main, frame.plan.channelMode);
    const std::array<double*, 2> audio{ selected.left, selected.right };
    for (std::size_t index = 0; index < frame.main.frameCount; ++index)
    {
        UpdateInputLevel(selected.Peak(index));
        const auto amount = amount_.NextValue();
        const auto baselineMix = std::min(amount * 4.0, 1.0);
        const auto levelLoss = 1.0 - linearLevelGain_;
        for (std::size_t channel = 0; channel < channels_.size(); ++channel)
        {
            if (audio[channel] != nullptr)
            {
                audio[channel][index] = ProcessChannelSample(
                    audio[channel][index], channels_[channel], amount, baselineMix, levelLoss);
            }
        }
    }
}

PolishAirProcessor::BiquadCoefficients PolishAirProcessor::CompileBell(
    double frequencyHz,
    double q,
    double gainDb,
    double sampleRate) noexcept
{
    const auto omega = 2.0 * std::numbers::pi * frequencyHz / sampleRate;
    const auto sine = std::sin(omega);
    const auto cosine = std::cos(omega);
    const auto alpha = sine / (2.0 * q);
    const auto amplitude = std::pow(10.0, gainDb / 40.0);
    const auto inverseA0 = 1.0 / (1.0 + alpha / amplitude);
    return {
        (1.0 + alpha * amplitude) * inverseA0,
        -2.0 * cosine * inverseA0,
        (1.0 - alpha * amplitude) * inverseA0,
        -2.0 * cosine * inverseA0,
        (1.0 - alpha / amplitude) * inverseA0
    };
}

double PolishAirProcessor::ProcessBiquad(
    double input,
    const BiquadCoefficients& coefficients,
    BiquadState& state) noexcept
{
    const auto output = coefficients.b0 * input + state.z1;
    state.z1 = coefficients.b1 * input - coefficients.a1 * output + state.z2;
    state.z2 = coefficients.b2 * input - coefficients.a2 * output;
    return output;
}

void PolishAirProcessor::PrepareFilters(double sampleRate) noexcept
{
    baselineHighPassCoefficient_ = onepole::FilterCoefficient(
        kConfig.baselineHighPassHz,
        sampleRate);
    lowerShelfCoefficient_ = onepole::FilterCoefficient(
        kConfig.lowerShelfFrequencyHz,
        sampleRate);
    upperShelfCoefficient_ = onepole::FilterCoefficient(
        kConfig.upperShelfFrequencyHz,
        sampleRate);
    levelShelfCoefficient_ = onepole::FilterCoefficient(
        kConfig.levelShelfFrequencyHz,
        sampleRate);
    nonlinearSourceHighPassCoefficient_ = onepole::FilterCoefficient(
        kConfig.nonlinearSourceHighPassHz,
        sampleRate);
    nonlinearOutputHighPassCoefficient_ = onepole::FilterCoefficient(
        kConfig.nonlinearOutputHighPassHz,
        sampleRate);
    presencePeakCoefficients_ = CompileBell(
        kConfig.presencePeakFrequencyHz,
        kConfig.presencePeakQ,
        kConfig.presencePeakGainDb,
        sampleRate);
    presenceNotchCoefficients_ = CompileBell(
        kConfig.presenceNotchFrequencyHz,
        kConfig.presenceNotchQ,
        kConfig.presenceNotchGainDb,
        sampleRate);
    levelBellCoefficients_ = CompileBell(
        kConfig.levelBellFrequencyHz,
        kConfig.levelBellQ,
        kConfig.levelBellGainDb,
        sampleRate);
}

void PolishAirProcessor::UpdateInputLevel(double peak) noexcept
{
    // Recurring peaks refresh the hold without modulating the contour on each sine cycle.
    const auto previousPeak = inputPeak_;
    if (peak >= inputPeak_ * 0.999)
    {
        inputPeak_ = std::max(inputPeak_, peak);
        peakHoldRemaining_ = peakHoldSamples_;
    }
    else if (peakHoldRemaining_ > 0)
    {
        --peakHoldRemaining_;
    }
    else
    {
        inputPeak_ += peakReleaseCoefficient_ * (peak - inputPeak_);
    }
    if (inputPeak_ != previousPeak)
    {
        const auto levelDb = 20.0 * std::log10(std::max(inputPeak_, 1.0e-8));
        const auto position = std::clamp((levelDb + 30.0) / 30.0, 0.0, 1.0);
        const auto& c = kConfig.linearLevelLaw;
        linearLevelGain_ = std::min(1.0, 1.0 + position * (c[0] + c[1] * position));
    }
}

double PolishAirProcessor::ProcessLinearAddition(
    double input, Channel& state, double levelLoss) noexcept
{
    const auto lowerShelf = onepole::HighPass(
        input,
        lowerShelfCoefficient_,
        state.lowerShelf);
    const auto upperShelf = onepole::HighPass(
        input,
        upperShelfCoefficient_,
        state.upperShelf);
    const auto levelShelf = onepole::HighPass(
        input,
        levelShelfCoefficient_,
        state.levelShelf);
    const auto presencePeak = ProcessBiquad(
        input,
        presencePeakCoefficients_,
        state.presencePeak);
    const auto presenceContour = ProcessBiquad(
        presencePeak,
        presenceNotchCoefficients_,
        state.presenceNotch);
    const auto levelBell = ProcessBiquad(
        input,
        levelBellCoefficients_,
        state.levelBell);
    return kConfig.directGain * input +
        kConfig.lowerShelfGain * lowerShelf +
        kConfig.upperShelfGain * upperShelf +
        (presenceContour - input) -
        levelLoss * (kConfig.levelShelfGain * levelShelf + input - levelBell);
}

double PolishAirProcessor::ProcessOddHarmonics(double input, Channel& state) noexcept
{
    const auto nonlinearSource = onepole::HighPass(
        input,
        nonlinearSourceHighPassCoefficient_,
        state.nonlinearSourceHighPass);
    const auto squaredSource = nonlinearSource * nonlinearSource;
    const auto cubedSource = squaredSource * nonlinearSource;
    const auto oddHarmonics = onepole::HighPass(
        cubedSource,
        nonlinearOutputHighPassCoefficient_,
        state.nonlinearOutputHighPass);
    return oddHarmonics;
}

double PolishAirProcessor::ProcessChannelSample(
    double input, Channel& state, double amount, double baselineMix, double levelLoss) noexcept
{
    const auto baseline = onepole::HighPass(
        input, baselineHighPassCoefficient_, state.baselineHighPass);
    const auto linearAddition = ProcessLinearAddition(input, state, levelLoss);
    // Harmonics stay outside levelLoss: it contracts only the linear upper contour.
    const auto oddHarmonics = ProcessOddHarmonics(input, state);
    return input + baselineMix * (baseline - input) +
        amount * (linearAddition - kConfig.oddHarmonicGain * oddHarmonics);
}

} // namespace consolidator::max::dsp::processors
