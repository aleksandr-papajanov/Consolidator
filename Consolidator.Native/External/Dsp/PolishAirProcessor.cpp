#include "PolishAirProcessor.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <numbers>

#include "DspMath.h"
#include "DspOnePole.h"
#include "DspProcessingPrimitives.h"

namespace consolidator::max::dsp
{

namespace
{

constexpr std::array<double, 5> kLevelAnchors{
    -30.0, -18.0, -12.0, -6.0, 0.0
};

double LinearGainForLevel(
    double levelDb,
    const PolishAirTuning& tuning) noexcept
{
    if (levelDb <= kLevelAnchors.front())
    {
        return tuning.linearLevelGainAnchors.front();
    }
    for (std::size_t index = 1; index < kLevelAnchors.size(); ++index)
    {
        if (levelDb <= kLevelAnchors[index])
        {
            const auto ratio = (levelDb - kLevelAnchors[index - 1]) /
                (kLevelAnchors[index] - kLevelAnchors[index - 1]);
            return tuning.linearLevelGainAnchors[index - 1] + ratio *
                (tuning.linearLevelGainAnchors[index] -
                    tuning.linearLevelGainAnchors[index - 1]);
        }
    }
    return tuning.linearLevelGainAnchors.back();
}

} // namespace

void PolishAirProcessor::SetTuningBeforePrepare(
    const PolishAirTuning& tuning) noexcept
{
    tuning_ = tuning;
}

void PolishAirProcessor::Prepare(const DspProcessingContext& context) noexcept
{
    amount_.Prepare(context);
    baselineHighPassCoefficient_ = onepole::FilterCoefficient(
        tuning_.baselineHighPassHz,
        context.sampleRate);
    lowerShelfCoefficient_ = onepole::FilterCoefficient(
        tuning_.lowerShelfFrequencyHz,
        context.sampleRate);
    middleShelfCoefficient_ = onepole::FilterCoefficient(
        tuning_.middleShelfFrequencyHz,
        context.sampleRate);
    upperShelfCoefficient_ = onepole::FilterCoefficient(
        tuning_.upperShelfFrequencyHz,
        context.sampleRate);
    levelShelfCoefficient_ = onepole::FilterCoefficient(
        tuning_.levelShelfFrequencyHz,
        context.sampleRate);
    nonlinearSourceHighPassCoefficient_ = onepole::FilterCoefficient(
        tuning_.nonlinearSourceHighPassHz,
        context.sampleRate);
    nonlinearOutputHighPassCoefficient_ = onepole::FilterCoefficient(
        tuning_.nonlinearOutputHighPassHz,
        context.sampleRate);
    evenHarmonicHighPassCoefficient_ = onepole::FilterCoefficient(
        tuning_.evenHarmonicHighPassHz,
        context.sampleRate);
    presencePeakCoefficients_ = CompileBell(
        tuning_.presencePeakFrequencyHz,
        tuning_.presencePeakQ,
        tuning_.presencePeakGainDb,
        context.sampleRate);
    presenceNotchCoefficients_ = CompileBell(
        tuning_.presenceNotchFrequencyHz,
        tuning_.presenceNotchQ,
        tuning_.presenceNotchGainDb,
        context.sampleRate);
    levelBellCoefficients_ = CompileBell(
        tuning_.levelBellFrequencyHz,
        tuning_.levelBellQ,
        tuning_.levelBellGainDb,
        context.sampleRate);
    peakReleaseCoefficient_ = TimeCoefficient(2.0, context.sampleRate);
    peakHoldSamples_ = static_cast<std::size_t>(context.sampleRate);
}

void PolishAirProcessor::Reset() noexcept
{
    amount_.Reset();
    channels_ = {};
    inputPeak_ = 0.0;
    peakHoldRemaining_ = 0;
}

void PolishAirProcessor::SetParameters(const DspSnapshot& snapshot) noexcept
{
    const auto amount = std::clamp(
        static_cast<double>(snapshot.polishAir),
        0.0,
        1.0);
    amount_.SetTarget(amount);
    targetAmountGain_ = std::pow(amount, tuning_.amountExponent);
}

void PolishAirProcessor::Process(DspProcessingFrame& frame) noexcept
{
    const auto selected = SelectChannels(frame.main, frame.plan.channelMode);
    const std::array<double*, 2> audio{ selected.left, selected.right };
    for (std::size_t index = 0; index < frame.main.frameCount; ++index)
    {
        auto peak = 0.0;
        for (const auto* channel : audio)
        {
            if (channel != nullptr)
            {
                peak = std::max(peak, std::abs(channel[index]));
            }
        }
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
        const auto levelDb = 20.0 * std::log10(std::max(inputPeak_, 1.0e-8));
        const auto linearLevelGain = LinearGainForLevel(levelDb, tuning_);
        const auto amountGain = amount_.IsSmoothing()
            ? std::pow(
                std::clamp(amount_.NextValue(), 0.0, 1.0),
                tuning_.amountExponent)
            : targetAmountGain_;

        for (std::size_t channel = 0; channel < channels_.size(); ++channel)
        {
            if (audio[channel] == nullptr)
            {
                continue;
            }
            auto& state = channels_[channel];
            const auto input = audio[channel][index];
            const auto baseline = onepole::HighPass(
                input,
                baselineHighPassCoefficient_,
                state.baselineHighPass);
            const auto lowerShelf = onepole::HighPass(
                input,
                lowerShelfCoefficient_,
                state.lowerShelf);
            const auto middleShelf = onepole::HighPass(
                input,
                middleShelfCoefficient_,
                state.middleShelf);
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
            const auto nonlinearSource = onepole::HighPass(
                input,
                nonlinearSourceHighPassCoefficient_,
                state.nonlinearSourceHighPass);
            const auto squaredSource = nonlinearSource * nonlinearSource;
            const auto cubedSource = squaredSource * nonlinearSource;
            const auto oddGenerated = cubedSource * (
                1.0 + tuning_.fifthOrderGain * squaredSource +
                tuning_.seventhOrderGain * squaredSource * squaredSource);
            const auto oddHarmonics = onepole::HighPass(
                oddGenerated,
                nonlinearOutputHighPassCoefficient_,
                state.nonlinearOutputHighPass);
            const auto evenGenerated = input * input;
            const auto evenHarmonics = onepole::HighPass(
                evenGenerated,
                evenHarmonicHighPassCoefficient_,
                state.evenHarmonicHighPass);
            const auto baselineMix = std::min(amountGain * 4.0, 1.0);
            audio[channel][index] = input + baselineMix * (baseline - input) +
                amountGain * (
                tuning_.directGain * input +
                tuning_.lowerShelfGain * lowerShelf +
                    tuning_.middleShelfGain * middleShelf +
                    tuning_.upperShelfGain * upperShelf +
                    (presenceContour - input) +
                (linearLevelGain - 1.0) *
                    tuning_.levelShelfGain * levelShelf -
                (1.0 - linearLevelGain) * (input - levelBell) -
                tuning_.oddHarmonicGain * oddHarmonics +
                tuning_.evenHarmonicGain * evenHarmonics);
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

} // namespace consolidator::max::dsp
