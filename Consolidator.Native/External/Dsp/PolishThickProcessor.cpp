#include "PolishThickProcessor.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "DspMath.h"
#include "DspOnePole.h"
#include "DspProcessingPrimitives.h"

namespace consolidator::max::dsp
{

namespace
{

constexpr std::size_t kDenseMaximumHarmonic = 159;
constexpr std::array<double, 7> kBodyLevelAnchors{
    -30.0, -18.0, -12.0, -9.0, -6.0, -3.0, 0.0
};
double BodyMixForLevel(
    double levelDb,
    const PolishThickTuning& tuning) noexcept
{
    if (levelDb <= kBodyLevelAnchors.front())
    {
        return tuning.bodyMixAnchors.front();
    }
    for (std::size_t index = 1; index < kBodyLevelAnchors.size(); ++index)
    {
        if (levelDb <= kBodyLevelAnchors[index])
        {
            const auto ratio = (levelDb - kBodyLevelAnchors[index - 1]) /
                (kBodyLevelAnchors[index] - kBodyLevelAnchors[index - 1]);
            return tuning.bodyMixAnchors[index - 1] + ratio *
                (tuning.bodyMixAnchors[index] - tuning.bodyMixAnchors[index - 1]);
        }
    }
    return tuning.bodyMixAnchors.back();
}

double GenerateDenseHarmonics(
    double sample,
    const PolishThickTuning& tuning) noexcept
{
    auto previous = 1.0;
    auto current = sample;
    auto output = 0.0;
    auto harmonicWeight = 1.0;
    for (std::size_t harmonic = 2;
        harmonic < kDenseMaximumHarmonic;
        harmonic += 2)
    {
        const auto even = 2.0 * sample * current - previous;
        output += harmonicWeight * tuning.evenHarmonicWeight * even;
        previous = current;
        current = even;

        const auto odd = 2.0 * sample * current - previous;
        output += harmonicWeight * odd;
        previous = current;
        current = odd;
        harmonicWeight *= tuning.denseHarmonicDecay;
    }
    return tuning.denseHarmonicGain * output;
}

double GenerateBass(
    double sample,
    double scale,
    double amount,
    const PolishThickTuning& tuning,
    double& positiveFeedback,
    double& negativeFeedback) noexcept
{
    const auto safeScale = std::max(scale, 1.0e-8);
    const auto normalized = std::clamp(sample / safeScale, -1.0, 1.0);
    const auto loopGain = 0.56 + (tuning.feedbackLoopGain - 0.56) * amount;

    // Opposite-polarity loops make odd orders dominant while retaining a
    // quieter even-order residue. Recursion continues beyond Nyquist, whose
    // folded products provide the deliberately dense saturation texture.
    positiveFeedback = std::clamp(
        normalized + loopGain * normalized * positiveFeedback,
        -4.0,
        4.0);
    negativeFeedback = std::clamp(
        -normalized + loopGain * -normalized * negativeFeedback,
        -4.0,
        4.0);
    const auto oddResidue = 0.5 * (positiveFeedback - negativeFeedback) - normalized;
    const auto evenResidue = 0.5 * (positiveFeedback + negativeFeedback);
    const auto denseResidue = GenerateDenseHarmonics(normalized, tuning);
    return safeScale * (
        oddResidue + tuning.evenHarmonicWeight * evenResidue + denseResidue);
}

double GenerateLowLevelBass(double sample, double scale) noexcept
{
    const auto safeScale = std::max(scale, 1.0e-8);
    const auto normalized = std::clamp(sample / safeScale, -1.0, 1.0);
    return 0.5 * safeScale * (2.0 * normalized * normalized - 1.0);
}

double GenerateSubHarmonics(double sample, double scale) noexcept
{
    const auto safeScale = std::max(scale, 1.0e-8);
    const auto normalized = std::clamp(sample / safeScale, -1.0, 1.0);
    const auto second = 2.0 * normalized * normalized - 1.0;
    const auto third = 2.0 * normalized * second - normalized;
    const auto fourth = 2.0 * normalized * third - second;
    const auto fifth = 2.0 * normalized * fourth - third;
    return 0.5 * safeScale * (
        second + 0.56 * third + 0.065 * fourth + 0.12 * fifth);
}

} // namespace

void PolishThickProcessor::SetTuningBeforePrepare(
    const PolishThickTuning& tuning) noexcept
{
    tuning_ = tuning;
}

void PolishThickProcessor::Prepare(const DspProcessingContext& context)
{
    amount_.Prepare(context);
    bodyPeak_.Prepare(context);
    subNotch_.Prepare(context);
    positiveSubShoulder_.Prepare(context);
    positiveBodyPeak_.Prepare(context);
    bodyLeft_.assign(context.maximumFrameCount, 0.0);
    bodyRight_.assign(context.maximumFrameCount, 0.0);
    positiveBodyLeft_.assign(context.maximumFrameCount, 0.0);
    positiveBodyRight_.assign(context.maximumFrameCount, 0.0);
    const auto bellType = static_cast<std::uint32_t>(DspFilterType::Bell);
    bodyPeak_.SetParameters({
        1, bellType, static_cast<float>(tuning_.bodyPeakFrequency),
        static_cast<float>(tuning_.bodyPeakGainDb),
        static_cast<float>(tuning_.bodyPeakQ), 0.7F });
    subNotch_.SetParameters({
        1, bellType, static_cast<float>(tuning_.subNotchFrequency),
        static_cast<float>(tuning_.subNotchGainDb),
        static_cast<float>(tuning_.subNotchQ), 0.7F });
    positiveSubShoulder_.SetParameters({
        1, bellType, static_cast<float>(tuning_.positiveShoulderFrequency),
        static_cast<float>(tuning_.positiveShoulderGainDb),
        static_cast<float>(tuning_.positiveShoulderQ), 0.7F });
    positiveBodyPeak_.SetParameters({
        1, bellType, static_cast<float>(tuning_.positivePeakFrequency),
        static_cast<float>(tuning_.positivePeakGainDb),
        static_cast<float>(tuning_.positivePeakQ), 0.7F });
    // The source is limited below the 60 Hz reference setting. Generated
    // harmonics begin above that range and remain available into the mids.
    rumbleCoefficient_ = onepole::FilterCoefficient(20.0, context.sampleRate);
    sourceCoefficient_ = onepole::FilterCoefficient(tuning_.sourceCutoffHz, context.sampleRate);
    highPassCoefficient_ = onepole::FilterCoefficient(tuning_.harmonicHighPassHz, context.sampleRate);
    lowPassCoefficient_ = onepole::FilterCoefficient(tuning_.harmonicLowPassHz, context.sampleRate);
    subHarmonicSourceCoefficient_ = onepole::FilterCoefficient(
        tuning_.subHarmonicCutoffHz,
        context.sampleRate);
    energyCoefficient_ = TimeCoefficient(0.200, context.sampleRate);
    fastEnergyCoefficient_ = TimeCoefficient(0.020, context.sampleRate);
    peakAttackCoefficient_ = TimeCoefficient(0.001, context.sampleRate);
    peakReleaseCoefficient_ = TimeCoefficient(0.120, context.sampleRate);
    transientPeakReleaseCoefficient_ = TimeCoefficient(0.030, context.sampleRate);
    transientGainAttackCoefficient_ = TimeCoefficient(0.0005, context.sampleRate);
    transientGainReleaseCoefficient_ = TimeCoefficient(0.080, context.sampleRate);
    gainAttackCoefficient_ = TimeCoefficient(0.001, context.sampleRate);
    gainReleaseCoefficient_ = TimeCoefficient(0.080, context.sampleRate);
}

void PolishThickProcessor::Reset() noexcept
{
    amount_.Reset();
    bodyPeak_.Reset();
    subNotch_.Reset();
    positiveSubShoulder_.Reset();
    positiveBodyPeak_.Reset();
    channels_ = {};
    energy_ = 0.0;
    mainEnergy_ = 0.0;
    energyWeight_ = 0.0;
    wetEnergy_ = 0.0;
    fastWetEnergy_ = 0.0;
    fastEnergyWeight_ = 0.0;
    harmonicGain_ = 1.0;
    sourcePeak_ = 0.0;
    fullBandPeak_ = 0.0;
    transientPeak_ = 0.0;
    transientGain_ = 1.0;
    bodyLevelMix_ = tuning_.bodyMixAnchors.front();
    positiveLevelMix_ = 0.0;
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
        bodyLeft_[index] = audio[0] != nullptr ? audio[0][index] : 0.0;
        bodyRight_[index] = audio[1] != nullptr ? audio[1][index] : 0.0;
        positiveBodyLeft_[index] = bodyLeft_[index];
        positiveBodyRight_[index] = bodyRight_[index];
    }
    auto body = DspAudioBuffer{
        bodyLeft_.data(),
        bodyRight_.data(),
        frame.main.frameCount
    };
    bodyPeak_.Process(body, frame.plan.channelMode);
    subNotch_.Process(body, frame.plan.channelMode);
    auto positiveBody = DspAudioBuffer{
        positiveBodyLeft_.data(),
        positiveBodyRight_.data(),
        frame.main.frameCount
    };
    positiveSubShoulder_.Process(positiveBody, frame.plan.channelMode);
    positiveBodyPeak_.Process(positiveBody, frame.plan.channelMode);
    for (std::size_t index = 0; index < frame.main.frameCount; ++index)
    {
        std::array<double, 2> bass{};
        auto energy = 0.0;
        auto mainEnergy = 0.0;
        for (std::size_t channel = 0; channel < channels_.size(); ++channel)
        {
            if (audio[channel] != nullptr)
            {
                auto& state = channels_[channel];
                const auto input = audio[channel][index];
                mainEnergy = std::max(mainEnergy, input * input);
                const auto source = onepole::HighPass(input, rumbleCoefficient_, state.rumble);
                bass[channel] = onepole::LowPass(source, sourceCoefficient_, state.source);
                energy = std::max(energy, bass[channel] * bass[channel]);
            }
        }
        energy_ += energyCoefficient_ * (energy - energy_);
        mainEnergy_ += energyCoefficient_ * (mainEnergy - mainEnergy_);
        energyWeight_ += energyCoefficient_ * (1.0 - energyWeight_);
        const auto meanEnergy = energy_ / std::max(energyWeight_, 1.0e-12);
        const auto meanMainEnergy = mainEnergy_ / std::max(energyWeight_, 1.0e-12);
        const auto bassPresence = std::clamp(
            std::sqrt(meanEnergy / std::max(meanMainEnergy, 1.0e-10)),
            0.0,
            1.0);
        const auto peak = std::sqrt(energy);
        const auto peakCoefficient = peak > sourcePeak_ ? peakAttackCoefficient_ : peakReleaseCoefficient_;
        sourcePeak_ += peakCoefficient * (peak - sourcePeak_);
        // Catch new attacks without waiting for the long-term RMS reference.
        const auto scale = std::max(sourcePeak_, std::sqrt(std::max(2.0 * meanEnergy, 1.0e-10)));
        const auto amount = std::clamp(amount_.NextValue(), 0.0, 1.0);
        const auto mainPeak = std::sqrt(mainEnergy);
        fullBandPeak_ = std::max(
            mainPeak,
            fullBandPeak_ + peakReleaseCoefficient_ * (mainPeak - fullBandPeak_));
        const auto rawFullBandLevel = std::max(
            std::sqrt(2.0 * meanMainEnergy),
            fullBandPeak_);
        const auto fullBandLevel = std::clamp(
            rawFullBandLevel,
            0.0,
            1.0);
        const auto bodyControlLevel = std::sqrt(std::max(
            2.0 * meanMainEnergy,
            1.0e-16));
        const auto bassReferenceLevel = std::sqrt(std::max(
            2.0 * meanEnergy,
            1.0e-16));
        transientPeak_ = std::max(
            peak,
            transientPeak_ + transientPeakReleaseCoefficient_ *
                (peak - transientPeak_));
        const auto transientCrest = transientPeak_ /
            std::max(bassReferenceLevel, 1.0e-8);
        auto transientGainTarget = 1.0;
        if (transientCrest > tuning_.transientCrestThreshold)
        {
            const auto crestGain = tuning_.transientCrestThreshold /
                transientCrest;
            transientGainTarget = std::max(
                tuning_.transientMinimumGain,
                std::sqrt(crestGain * std::sqrt(crestGain)));
        }
        const auto transientGainCoefficient =
            transientGainTarget < transientGain_
            ? transientGainAttackCoefficient_
            : transientGainReleaseCoefficient_;
        transientGain_ += transientGainCoefficient *
            (transientGainTarget - transientGain_);
        const auto bodyControlLevelDb = 20.0 * std::log10(std::max(
            bodyControlLevel,
            1.0e-8));
        const auto bodyLevelMixTarget = BodyMixForLevel(bodyControlLevelDb, tuning_);
        bodyLevelMix_ += fastEnergyCoefficient_ *
            (bodyLevelMixTarget - bodyLevelMix_);
        const auto positiveLevelMixTarget = std::clamp(
            bodyControlLevelDb / 10.0,
            0.0,
            1.0);
        positiveLevelMix_ += fastEnergyCoefficient_ *
            (positiveLevelMixTarget - positiveLevelMix_);
        const auto levelSquared = fullBandLevel * fullBandLevel;
        const auto levelFourth = levelSquared * levelSquared;
        const auto levelEighth = levelFourth * levelFourth;
        const auto levelSixteenth = levelEighth * levelEighth;
        const auto levelTwentyFourth = levelSixteenth * levelEighth;
        const auto lowLevelDrive = tuning_.lowLevelDrive * fullBandLevel;
        const auto richLevelDrive = tuning_.richLevelDrive * levelTwentyFourth;
        const auto intensity = amount * amount * (1.0 + 25.0 * amount) *
            bassPresence;
        std::array<double, 2> wet{};
        std::array<double, 2> lowLevelWet{};
        std::array<double, 2> subHarmonicWet{};
        auto wetEnergy = 0.0;
        for (std::size_t channel = 0; channel < channels_.size(); ++channel)
        {
            if (audio[channel] == nullptr)
            {
                continue;
            }
            auto& state = channels_[channel];
            const auto generated = GenerateBass(
                bass[channel],
                scale,
                amount,
                tuning_,
                state.positiveFeedback,
                state.negativeFeedback);
            const auto upperBass = onepole::HighPass(generated, highPassCoefficient_, state.highPass);
            wet[channel] = onepole::LowPass(upperBass, lowPassCoefficient_, state.lowPass);
            const auto lowLevelGenerated = GenerateLowLevelBass(bass[channel], scale);
            const auto subHarmonicSource = onepole::LowPass(
                bass[channel],
                subHarmonicSourceCoefficient_,
                state.subHarmonicSource);
            const auto subHarmonics = GenerateSubHarmonics(
                subHarmonicSource,
                scale);
            const auto lowLevelUpperBass = onepole::HighPass(
                lowLevelGenerated,
                highPassCoefficient_,
                state.lowLevelHighPass);
            lowLevelWet[channel] = onepole::LowPass(
                lowLevelUpperBass,
                lowPassCoefficient_,
                state.lowLevelLowPass);
            const auto subHarmonicUpperBass = onepole::HighPass(
                subHarmonics,
                highPassCoefficient_,
                state.subHarmonicHighPass);
            subHarmonicWet[channel] = onepole::LowPass(
                subHarmonicUpperBass,
                lowPassCoefficient_,
                state.subHarmonicLowPass);
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
            desiredGain = std::clamp(
                calibration * upward * burstReduction / std::max(intensity, 1.0),
                0.01,
                4.0);
        }
        const auto gainCoefficient = desiredGain < harmonicGain_ ? gainAttackCoefficient_ : gainReleaseCoefficient_;
        harmonicGain_ += gainCoefficient * (desiredGain - harmonicGain_);
        for (std::size_t channel = 0; channel < channels_.size(); ++channel)
        {
            if (audio[channel] != nullptr)
            {
                const auto bodySample = channel == 0
                    ? bodyLeft_[index]
                    : bodyRight_[index];
                const auto positiveBodySample = channel == 0
                    ? positiveBodyLeft_[index]
                    : positiveBodyRight_[index];
                const auto levelBodySample = bodySample + positiveLevelMix_ *
                    (positiveBodySample - bodySample);
                const auto bodyWet = amount * bodyLevelMix_ *
                    (levelBodySample - audio[channel][index]);
                const auto harmonicEnhancement = intensity * harmonicGain_ * (
                    richLevelDrive * wet[channel] +
                    lowLevelDrive * lowLevelWet[channel]);
                const auto subEnhancement = intensity *
                    tuning_.subHarmonicDrive * subHarmonicWet[channel];
                const auto transientWetGain = 1.0 + amount *
                    (transientGain_ - 1.0);
                const auto transientBodyGain = 1.0 + amount *
                    tuning_.transientBodyControl * (transientGain_ - 1.0);
                const auto controlledEnhancement =
                    transientBodyGain * bodyWet +
                    transientWetGain * harmonicEnhancement +
                    subEnhancement;
                const auto enhanced = audio[channel][index] +
                    controlledEnhancement;
                const auto limited = std::clamp(enhanced, -1.0, 1.0);
                audio[channel][index] = enhanced + amount *
                    (limited - enhanced);
            }
        }
    }
}

} // namespace consolidator::max::dsp
