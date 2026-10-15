#include "PolishAirProcessor.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <numbers>

#include "../../Primitives/Math.h"
#include "../../Primitives/Filters/OnePole.h"
#include "../../Primitives/ChannelView.h"

namespace consolidator::audio::algorithms
{

void PolishAirProcessor::Prepare(const ProcessingContext& context) noexcept
{
    amount_.Prepare(context);
    PrepareFilters(context.sampleRate);
    peakReleaseCoefficient_ = primitives::TimeCoefficient(2.0, context.sampleRate);
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

void PolishAirProcessor::SetAmount(double value) noexcept
{
    const auto amount = std::clamp(
        static_cast<double>(value),
        0.0,
        1.0);
    amount_.SetTarget(amount);
}

void PolishAirProcessor::Process(AudioBuffer buffer, ChannelMode mode) noexcept
{
    const auto selected = primitives::SelectChannels(buffer, mode);
    const std::array<double*, 2> audio{ selected.left, selected.right };
    for (std::size_t index = 0; index < buffer.frameCount; ++index)
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

void PolishAirProcessor::PrepareFilters(double sampleRate) noexcept
{
    baselineHighPassCoefficient_ = primitives::filters::onepole::FilterCoefficient(
        kConfig.baselineHighPassHz,
        sampleRate);
    lowerShelfCoefficient_ = primitives::filters::onepole::FilterCoefficient(
        kConfig.lowerShelfFrequencyHz,
        sampleRate);
    upperShelfCoefficient_ = primitives::filters::onepole::FilterCoefficient(
        kConfig.upperShelfFrequencyHz,
        sampleRate);
    levelShelfCoefficient_ = primitives::filters::onepole::FilterCoefficient(
        kConfig.levelShelfFrequencyHz,
        sampleRate);
    nonlinearSourceHighPassCoefficient_ = primitives::filters::onepole::FilterCoefficient(
        kConfig.nonlinearSourceHighPassHz,
        sampleRate);
    nonlinearOutputHighPassCoefficient_ = primitives::filters::onepole::FilterCoefficient(
        kConfig.nonlinearOutputHighPassHz,
        sampleRate);
    presencePeakCoefficients_ = primitives::filters::DesignBell(
        kConfig.presencePeakFrequencyHz,
        kConfig.presencePeakQ,
        kConfig.presencePeakGainDb,
        sampleRate);
    presenceNotchCoefficients_ = primitives::filters::DesignBell(
        kConfig.presenceNotchFrequencyHz,
        kConfig.presenceNotchQ,
        kConfig.presenceNotchGainDb,
        sampleRate);
    levelBellCoefficients_ = primitives::filters::DesignBell(
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
    const auto lowerShelf = primitives::filters::onepole::HighPass(
        input,
        lowerShelfCoefficient_,
        state.lowerShelf);
    const auto upperShelf = primitives::filters::onepole::HighPass(
        input,
        upperShelfCoefficient_,
        state.upperShelf);
    const auto levelShelf = primitives::filters::onepole::HighPass(
        input,
        levelShelfCoefficient_,
        state.levelShelf);
    const auto presencePeak = state.presencePeak.Process(input, presencePeakCoefficients_);
    const auto presenceContour = state.presenceNotch.Process(presencePeak, presenceNotchCoefficients_);
    const auto levelBell = state.levelBell.Process(input, levelBellCoefficients_);
    return kConfig.directGain * input +
        kConfig.lowerShelfGain * lowerShelf +
        kConfig.upperShelfGain * upperShelf +
        (presenceContour - input) -
        levelLoss * (kConfig.levelShelfGain * levelShelf + input - levelBell);
}

double PolishAirProcessor::ProcessOddHarmonics(double input, Channel& state) noexcept
{
    const auto nonlinearSource = primitives::filters::onepole::HighPass(
        input,
        nonlinearSourceHighPassCoefficient_,
        state.nonlinearSourceHighPass);
    const auto squaredSource = nonlinearSource * nonlinearSource;
    const auto cubedSource = squaredSource * nonlinearSource;
    const auto oddHarmonics = primitives::filters::onepole::HighPass(
        cubedSource,
        nonlinearOutputHighPassCoefficient_,
        state.nonlinearOutputHighPass);
    return oddHarmonics;
}

double PolishAirProcessor::ProcessChannelSample(
    double input, Channel& state, double amount, double baselineMix, double levelLoss) noexcept
{
    const auto baseline = primitives::filters::onepole::HighPass(
        input, baselineHighPassCoefficient_, state.baselineHighPass);
    const auto linearAddition = ProcessLinearAddition(input, state, levelLoss);
    // Harmonics stay outside levelLoss: it contracts only the linear upper contour.
    const auto oddHarmonics = ProcessOddHarmonics(input, state);
    return input + baselineMix * (baseline - input) +
        amount * (linearAddition - kConfig.oddHarmonicGain * oddHarmonics);
}

} // namespace consolidator::audio::algorithms
