#include "PolishThickProcessor.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "../../Primitives/Math.h"
#include "../../Primitives/Filters/OnePole.h"
#include "../../Primitives/ChannelView.h"

namespace consolidator::audio::algorithms
{

namespace
{

double BodyMixForLevel(double amplitude, const PolishThickConfig& config) noexcept
{
    const auto x = std::sqrt(std::min(amplitude, 1.0));
    const auto& c = config.bodyMixLaw;
    return std::clamp(config.bodyMixFloor +
        (1.0 - x) * (c[0] + x * (c[1] + x * c[2])) / (1.0 + c[3] * x),
        config.bodyMixFloor, 1.0);
}

double GenerateDenseHarmonics(
    double sample, const PolishThickConfig& config, double tailWeight) noexcept
{
    // Fast doubling gives T79/T80 in seven steps. The finite geometric sum
    // preserves every original H2..H159 term, including intentional folding.
    auto current = 1.0;
    auto next = sample;
    constexpr std::size_t pairCount = 79;
    for (std::size_t bit = 64; bit != 0; bit >>= 1)
    {
        const auto even = 2.0 * current * current - 1.0;
        const auto odd = 2.0 * current * next - sample;
        const auto upper = 2.0 * next * next - 1.0;
        if ((pairCount & bit) != 0)
        {
            current = odd;
            next = upper;
        }
        else
        {
            current = even;
            next = odd;
        }
    }
    const auto t158 = 2.0 * current * current - 1.0;
    const auto t159 = 2.0 * current * next - sample;
    const auto t160 = 2.0 * next * next - 1.0;
    const auto t161 = 2.0 * sample * t160 - t159;
    const auto t2 = 2.0 * sample * sample - 1.0;
    const auto t3 = 2.0 * sample * t2 - sample;
    const auto decay = config.denseHarmonicDecay;
    const auto denominator = 1.0 - 2.0 * decay * t2 + decay * decay;
    const auto evenSum = t2 - decay - tailWeight * (t160 - decay * t158);
    const auto oddSum = t3 - decay * sample - tailWeight * (t161 - decay * t159);
    return config.denseHarmonicGain *
        (oddSum + config.evenHarmonicWeight * evenSum) / denominator;
}

double GenerateBass(
    double normalized,
    double scale,
    double loopGain,
    const PolishThickConfig& config,
    double tailWeight,
    double& positiveFeedback,
    double& negativeFeedback) noexcept
{
    // Opposite-polarity loops make odd orders dominant while retaining a
    // quieter even-order residue. Recursion continues beyond Nyquist, whose
    // folded products provide the deliberately dense saturation texture.
    const auto feedbackInput = loopGain * normalized;
    positiveFeedback = std::clamp(
        normalized + feedbackInput * positiveFeedback,
        -4.0,
        4.0);
    negativeFeedback = std::clamp(
        -normalized - feedbackInput * negativeFeedback,
        -4.0,
        4.0);
    const auto oddResidue = 0.5 * (positiveFeedback - negativeFeedback) - normalized;
    const auto evenResidue = 0.5 * (positiveFeedback + negativeFeedback);
    const auto denseResidue = GenerateDenseHarmonics(normalized, config, tailWeight);
    return scale * (
        oddResidue + config.evenHarmonicWeight * evenResidue + denseResidue);
}

double GenerateSubHarmonics(double normalized, double scale) noexcept
{
    const auto second = 2.0 * normalized * normalized - 1.0;
    const auto third = 2.0 * normalized * second - normalized;
    const auto fourth = 2.0 * normalized * third - second;
    const auto fifth = 2.0 * normalized * fourth - third;
    return 0.5 * scale * (
        second + 0.56 * third + 0.065 * fourth + 0.12 * fifth);
}

} // namespace

void PolishThickProcessor::Prepare(const ProcessingContext& context)
{
    amount_.Prepare(context);
    denseTailWeight_ = std::pow(kConfig.denseHarmonicDecay, 79.0);
    PrepareBodyFilters(context);
    PrepareSourceAndDynamics(context.sampleRate);
}

void PolishThickProcessor::Reset() noexcept
{
    amount_.Reset();
    bodyPeak_.Reset();
    subNotch_.Reset();
    positiveSubShoulder_.Reset();
    positiveBodyPeak_.Reset();
    channels_ = {};
    sourcePower_ = 0.0;
    fullBandPower_ = 0.0;
    sourceStartupWeight_ = 0.0;
    richWetPower_ = 0.0;
    fastRichWetPower_ = 0.0;
    wetStartupWeight_ = 0.0;
    harmonicGain_ = 1.0;
    sourcePeak_ = 0.0;
    fullBandPeak_ = 0.0;
    transientPeak_ = 0.0;
    transientGain_ = 1.0;
    bodyLevelMix_ = 1.0;
    positiveLevelMix_ = 0.0;
}

void PolishThickProcessor::SetAmount(double value) noexcept
{
    amount_.SetTarget(value);
}

void PolishThickProcessor::Process(AudioBuffer buffer, ChannelMode mode) noexcept
{
    const auto selected = primitives::SelectChannels(buffer, mode);
    const std::array<double*, 2> audio{ selected.left, selected.right };
    ProcessBodyPaths(audio, buffer.frameCount, mode);
    for (std::size_t index = 0; index < buffer.frameCount; ++index)
    {
        const auto source = AnalyseSource(audio, index);
        const auto controls = UpdateSourceControls(source);
        const auto bands = GenerateHarmonicBands(audio, source, controls);
        UpdateWetGain(source, controls, bands.richPower);
        MixOutputSample(audio, index, controls, bands);
    }
}

void PolishThickProcessor::PrepareBodyFilters(const ProcessingContext& context)
{
    bodyPeak_ = {};
    subNotch_ = {};
    positiveSubShoulder_ = {};
    positiveBodyPeak_ = {};
    bodyPeak_.Prepare(context);
    subNotch_.Prepare(context);
    positiveSubShoulder_.Prepare(context);
    positiveBodyPeak_.Prepare(context);
    bodyLeft_.assign(context.maximumFrameCount, 0.0);
    bodyRight_.assign(context.maximumFrameCount, 0.0);
    positiveBodyLeft_.assign(context.maximumFrameCount, 0.0);
    positiveBodyRight_.assign(context.maximumFrameCount, 0.0);
    const auto bellType = static_cast<std::uint32_t>(FilterType::Bell);
    bodyPeak_.SetParameters({
        1, bellType, static_cast<float>(kConfig.bodyPeakFrequency),
        static_cast<float>(kConfig.bodyPeakGainDb),
        static_cast<float>(kConfig.bodyPeakQ), 0.7F });
    subNotch_.SetParameters({
        1, bellType, static_cast<float>(kConfig.subNotchFrequency),
        static_cast<float>(kConfig.subNotchGainDb),
        static_cast<float>(kConfig.subNotchQ), 0.7F });
    positiveSubShoulder_.SetParameters({
        1, bellType, static_cast<float>(kConfig.positiveShoulderFrequency),
        static_cast<float>(kConfig.positiveShoulderGainDb),
        static_cast<float>(kConfig.positiveShoulderQ), 0.7F });
    positiveBodyPeak_.SetParameters({
        1, bellType, static_cast<float>(kConfig.positivePeakFrequency),
        static_cast<float>(kConfig.positivePeakGainDb),
        static_cast<float>(kConfig.positivePeakQ), 0.7F });
}

void PolishThickProcessor::PrepareSourceAndDynamics(double sampleRate) noexcept
{
    // The source is limited below the 60 Hz reference setting. Generated
    // harmonics begin above that range and remain available into the mids.
    rumbleCoefficient_ = primitives::filters::onepole::FilterCoefficient(20.0, sampleRate);
    sourceCoefficient_ = primitives::filters::onepole::FilterCoefficient(kConfig.sourceCutoffHz, sampleRate);
    highPassCoefficient_ = primitives::filters::onepole::FilterCoefficient(kConfig.harmonicHighPassHz, sampleRate);
    lowPassCoefficient_ = primitives::filters::onepole::FilterCoefficient(kConfig.harmonicLowPassHz, sampleRate);
    subHarmonicSourceCoefficient_ = primitives::filters::onepole::FilterCoefficient(
        kConfig.subHarmonicCutoffHz,
        sampleRate);
    energyCoefficient_ = primitives::TimeCoefficient(0.200, sampleRate);
    fastEnergyCoefficient_ = primitives::TimeCoefficient(0.020, sampleRate);
    peakAttackCoefficient_ = primitives::TimeCoefficient(0.001, sampleRate);
    peakReleaseCoefficient_ = primitives::TimeCoefficient(0.120, sampleRate);
    transientPeakReleaseCoefficient_ = primitives::TimeCoefficient(0.030, sampleRate);
    transientGainAttackCoefficient_ = primitives::TimeCoefficient(0.0005, sampleRate);
    transientGainReleaseCoefficient_ = primitives::TimeCoefficient(0.080, sampleRate);
    gainAttackCoefficient_ = primitives::TimeCoefficient(0.001, sampleRate);
    gainReleaseCoefficient_ = primitives::TimeCoefficient(0.080, sampleRate);
}

void PolishThickProcessor::ProcessBodyPaths(
    const std::array<double*, 2>& audio, std::size_t frameCount, ChannelMode mode) noexcept
{
    for (std::size_t index = 0; index < frameCount; ++index)
    {
        bodyLeft_[index] = audio[0] != nullptr ? audio[0][index] : 0.0;
        bodyRight_[index] = audio[1] != nullptr ? audio[1][index] : 0.0;
        positiveBodyLeft_[index] = bodyLeft_[index];
        positiveBodyRight_[index] = bodyRight_[index];
    }
    auto body = AudioBuffer{
        bodyLeft_.data(),
        bodyRight_.data(),
        frameCount
    };
    bodyPeak_.Process(body, mode);
    subNotch_.Process(body, mode);
    auto positiveBody = AudioBuffer{
        positiveBodyLeft_.data(),
        positiveBodyRight_.data(),
        frameCount
    };
    positiveSubShoulder_.Process(positiveBody, mode);
    positiveBodyPeak_.Process(positiveBody, mode);
}

PolishThickProcessor::SourceAnalysis PolishThickProcessor::AnalyseSource(
    const std::array<double*, 2>& audio, std::size_t index) noexcept
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
            const auto source = primitives::filters::onepole::HighPass(input, rumbleCoefficient_, state.rumble);
            bass[channel] = primitives::filters::onepole::LowPass(source, sourceCoefficient_, state.source);
            energy = std::max(energy, bass[channel] * bass[channel]);
        }
    }
    sourcePower_ += energyCoefficient_ * (energy - sourcePower_);
    fullBandPower_ += energyCoefficient_ * (mainEnergy - fullBandPower_);
    sourceStartupWeight_ += energyCoefficient_ * (1.0 - sourceStartupWeight_);
    const auto inverseEnergyWeight = 1.0 / std::max(sourceStartupWeight_, 1.0e-12);
    const auto meanEnergy = sourcePower_ * inverseEnergyWeight;
    const auto meanMainEnergy = fullBandPower_ * inverseEnergyWeight;
    const auto bassPresence = std::clamp(
        std::sqrt(meanEnergy / std::max(meanMainEnergy, 1.0e-10)),
        0.0,
        1.0);
    const auto peak = std::sqrt(energy);
    return { bass, meanEnergy, meanMainEnergy, inverseEnergyWeight, peak, bassPresence, mainEnergy };
}

PolishThickProcessor::SampleControls PolishThickProcessor::UpdateSourceControls(
    const SourceAnalysis& source) noexcept
{
    const auto peakCoefficient = source.bassPeak > sourcePeak_ ? peakAttackCoefficient_ : peakReleaseCoefficient_;
    sourcePeak_ += peakCoefficient * (source.bassPeak - sourcePeak_);
    // Catch new attacks without waiting for the long-term RMS reference.
    const auto bassReferenceLevel = std::sqrt(std::max(2.0 * source.bassMeanSquare, 1.0e-16));
    const auto scale = std::max(sourcePeak_, std::max(bassReferenceLevel, 1.0e-5));
    const auto inverseScale = 1.0 / scale;
    const auto amount = std::clamp(amount_.NextValue(), 0.0, 1.0);
    const auto loopGain = 0.56 + (kConfig.feedbackLoopGain - 0.56) * amount;
    const auto bodyControlLevel = std::sqrt(std::max(2.0 * source.fullBandMeanSquare, 1.0e-16));
    const auto mainPeak = std::sqrt(source.fullBandPower);
    fullBandPeak_ = std::max(
        mainPeak,
        fullBandPeak_ + peakReleaseCoefficient_ * (mainPeak - fullBandPeak_));
    const auto rawFullBandLevel = std::max(
        bodyControlLevel,
        fullBandPeak_);
    const auto fullBandLevel = std::clamp(
        rawFullBandLevel,
        0.0,
        1.0);
    UpdateCrestGain(source.bassPeak, bassReferenceLevel);
    UpdateBodyLevelMix(bodyControlLevel);
    const auto levelSquared = fullBandLevel * fullBandLevel;
    const auto levelFourth = levelSquared * levelSquared;
    const auto levelEighth = levelFourth * levelFourth;
    const auto levelSixteenth = levelEighth * levelEighth;
    const auto levelTwentyFourth = levelSixteenth * levelEighth;
    const auto lowLevelDrive = kConfig.lowLevelDrive * fullBandLevel;
    const auto richLevelDrive = kConfig.richLevelDrive * levelTwentyFourth;
    const auto intensity = amount * amount * (1.0 + 25.0 * amount) *
        source.bassPresence;
    return { amount, scale, inverseScale, loopGain, lowLevelDrive, richLevelDrive, intensity };
}

void PolishThickProcessor::UpdateCrestGain(double peak, double bassReferenceLevel) noexcept
{
    transientPeak_ = std::max(
        peak,
        transientPeak_ + transientPeakReleaseCoefficient_ *
            (peak - transientPeak_));
    const auto transientCrest = transientPeak_ /
        std::max(bassReferenceLevel, 1.0e-8);
    auto transientGainTarget = 1.0;
    if (transientCrest > kConfig.transientCrestThreshold)
    {
        const auto crestGain = kConfig.transientCrestThreshold /
            transientCrest;
        transientGainTarget = std::max(
            kConfig.transientMinimumGain,
            std::sqrt(crestGain * std::sqrt(crestGain)));
    }
    const auto transientGainCoefficient =
        transientGainTarget < transientGain_
        ? transientGainAttackCoefficient_
        : transientGainReleaseCoefficient_;
    transientGain_ += transientGainCoefficient *
        (transientGainTarget - transientGain_);
}

void PolishThickProcessor::UpdateBodyLevelMix(double bodyControlLevel) noexcept
{
    const auto bodyLevelMixTarget = BodyMixForLevel(bodyControlLevel, kConfig);
    bodyLevelMix_ += fastEnergyCoefficient_ *
        (bodyLevelMixTarget - bodyLevelMix_);
    const auto positiveLevelMixTarget = bodyControlLevel > 1.0
        ? std::min(2.0 * std::log10(bodyControlLevel), 1.0) : 0.0;
    positiveLevelMix_ += fastEnergyCoefficient_ *
        (positiveLevelMixTarget - positiveLevelMix_);
}

PolishThickProcessor::HarmonicBands PolishThickProcessor::GenerateHarmonicBands(
    const std::array<double*, 2>& audio, const SourceAnalysis& source, const SampleControls& controls) noexcept
{
    const auto& bass = source.bass;
    const auto scale = controls.scale;
    const auto inverseScale = controls.inverseScale;
    const auto loopGain = controls.loopGain;
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
        const auto normalized = std::clamp(bass[channel] * inverseScale, -1.0, 1.0);
        const auto generated = GenerateBass(
            normalized,
            scale,
            loopGain,
            kConfig,
            denseTailWeight_,
            state.positiveFeedback,
            state.negativeFeedback);
        const auto upperBass = primitives::filters::onepole::HighPass(generated, highPassCoefficient_, state.highPass);
        wet[channel] = primitives::filters::onepole::LowPass(upperBass, lowPassCoefficient_, state.lowPass);
        const auto lowLevelGenerated = 0.5 * scale * (2.0 * normalized * normalized - 1.0);
        const auto subHarmonicSource = primitives::filters::onepole::LowPass(
            bass[channel],
            subHarmonicSourceCoefficient_,
            state.subHarmonicSource);
        const auto subHarmonics = GenerateSubHarmonics(
            std::clamp(subHarmonicSource * inverseScale, -1.0, 1.0),
            scale);
        const auto lowLevelUpperBass = primitives::filters::onepole::HighPass(
            lowLevelGenerated,
            highPassCoefficient_,
            state.lowLevelHighPass);
        lowLevelWet[channel] = primitives::filters::onepole::LowPass(
            lowLevelUpperBass,
            lowPassCoefficient_,
            state.lowLevelLowPass);
        const auto subHarmonicUpperBass = primitives::filters::onepole::HighPass(
            subHarmonics,
            highPassCoefficient_,
            state.subHarmonicHighPass);
        subHarmonicWet[channel] = primitives::filters::onepole::LowPass(
            subHarmonicUpperBass,
            lowPassCoefficient_,
            state.subHarmonicLowPass);
        wetEnergy = std::max(wetEnergy, wet[channel] * wet[channel]);
    }
    return { wet, lowLevelWet, subHarmonicWet, wetEnergy };
}

void PolishThickProcessor::UpdateWetGain(
    const SourceAnalysis& source, const SampleControls& controls, double wetEnergy) noexcept
{
    const auto inverseEnergyWeight = source.inverseStartupWeight;
    const auto meanEnergy = source.bassMeanSquare;
    const auto intensity = controls.intensity;
    richWetPower_ += energyCoefficient_ * (wetEnergy - richWetPower_);
    fastRichWetPower_ += fastEnergyCoefficient_ * (wetEnergy - fastRichWetPower_);
    wetStartupWeight_ += fastEnergyCoefficient_ * (1.0 - wetStartupWeight_);
    const auto meanWet = richWetPower_ * inverseEnergyWeight;
    const auto fastWet = fastRichWetPower_ / std::max(wetStartupWeight_, 1.0e-12);
    auto desiredGain = 1.0;
    if (meanEnergy > 1.0e-10 && fastWet > 1.0e-10)
    {
        // Lift the body/tail, but reduce gain on wet bursts instead of
        // retaining the boosted tail gain through the next attack.
        const auto calibration = std::clamp(std::sqrt(meanEnergy / std::max(meanWet, 1.0e-10)), 0.5, 3.0);
        const auto wetRatio = meanWet / fastWet;
        const auto upward = wetRatio > 1.0 ? std::min(std::sqrt(std::sqrt(wetRatio)), 1.5) : 1.0;
        const auto burstReduction = wetRatio < 1.0 ? std::sqrt(wetRatio) : 1.0;
        desiredGain = std::clamp(
            calibration * upward * burstReduction / std::max(intensity, 1.0),
            0.01,
            4.0);
    }
    const auto gainCoefficient = desiredGain < harmonicGain_ ? gainAttackCoefficient_ : gainReleaseCoefficient_;
    harmonicGain_ += gainCoefficient * (desiredGain - harmonicGain_);
}

void PolishThickProcessor::MixOutputSample(
    const std::array<double*, 2>& audio, std::size_t index,
    const SampleControls& controls, const HarmonicBands& bands) noexcept
{
    const auto amount = controls.amount;
    const auto lowLevelDrive = controls.lowLevelDrive;
    const auto richLevelDrive = controls.richLevelDrive;
    const auto intensity = controls.intensity;
    const auto& wet = bands.rich;
    const auto& lowLevelWet = bands.lowLevel;
    const auto& subHarmonicWet = bands.sub;
    const auto transientWetGain = 1.0 + amount * (transientGain_ - 1.0);
    const auto transientBodyGain = 1.0 + amount * kConfig.transientBodyControl * (transientGain_ - 1.0);
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
                kConfig.subHarmonicDrive * subHarmonicWet[channel];
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

} // namespace consolidator::audio::algorithms
