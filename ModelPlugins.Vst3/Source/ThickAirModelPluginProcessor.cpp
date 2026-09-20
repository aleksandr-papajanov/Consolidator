#include <JuceHeader.h>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#if defined(CONSOLIDATOR_THICK_MODEL)
#include "PolishThickProcessor.h"
#elif defined(CONSOLIDATOR_AIR_MODEL)
#include "PolishAirProcessor.h"
#else
#error A Polish model kind must be selected.
#endif

namespace consolidator::model_plugins
{

using namespace consolidator::max::dsp;

namespace
{

void AddFloat(
    std::vector<std::unique_ptr<juce::RangedAudioParameter>>& parameters,
    const char* id,
    const char* name,
    double minimum,
    double maximum,
    double defaultValue)
{
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(id, 1), name,
        juce::NormalisableRange<float>(
            static_cast<float>(minimum), static_cast<float>(maximum)),
        static_cast<float>(defaultValue)));
}

juce::AudioProcessorValueTreeState::ParameterLayout CreateParameterLayout()
{
#if defined(CONSOLIDATOR_THICK_MODEL)
    PolishThickTuning thick;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> values;
    AddFloat(values, "thick", "Thick", 0.0F, 1.0F, 0.0F);
    values.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("bypass", 1), "Bypass", false));

    AddFloat(values, "thick.bodyPeakFrequency", "Thick Body Peak Frequency", 35.0F, 120.0F, thick.bodyPeakFrequency);
    AddFloat(values, "thick.bodyPeakGainDb", "Thick Body Peak Gain", 0.0F, 30.0F, thick.bodyPeakGainDb);
    AddFloat(values, "thick.bodyPeakQ", "Thick Body Peak Q", 0.2F, 6.0F, thick.bodyPeakQ);
    AddFloat(values, "thick.subNotchFrequency", "Thick Sub Notch Frequency", 15.0F, 45.0F, thick.subNotchFrequency);
    AddFloat(values, "thick.subNotchGainDb", "Thick Sub Notch Gain", -30.0F, 0.0F, thick.subNotchGainDb);
    AddFloat(values, "thick.subNotchQ", "Thick Sub Notch Q", 0.2F, 8.0F, thick.subNotchQ);
    AddFloat(values, "thick.positiveShoulderFrequency",
        "Thick Positive Shoulder Frequency", 10.0F, 40.0F,
        thick.positiveShoulderFrequency);
    AddFloat(values, "thick.positiveShoulderGainDb",
        "Thick Positive Shoulder Gain", -15.0F, 24.0F,
        thick.positiveShoulderGainDb);
    AddFloat(values, "thick.positiveShoulderQ", "Thick Positive Shoulder Q", 0.2F, 6.0F, thick.positiveShoulderQ);
    AddFloat(values, "thick.positivePeakFrequency",
        "Thick Positive Peak Frequency", 25.0F, 100.0F,
        thick.positivePeakFrequency);
    AddFloat(values, "thick.positivePeakGainDb", "Thick Positive Peak Gain", -30.0F, 12.0F, thick.positivePeakGainDb);
    AddFloat(values, "thick.positivePeakQ", "Thick Positive Peak Q", 0.2F, 6.0F, thick.positivePeakQ);
    for (std::size_t index = 0; index < thick.bodyMixAnchors.size(); ++index)
    {
        const auto id = "thick.bodyMix" + std::to_string(index);
        const auto name = "Thick Body Mix " + std::to_string(index);
        AddFloat(values, id.c_str(), name.c_str(), 0.0F, 1.5F, thick.bodyMixAnchors[index]);
    }
    AddFloat(values, "thick.sourceCutoffHz", "Thick Source Cutoff", 25.0F, 120.0F, thick.sourceCutoffHz);
    AddFloat(values, "thick.harmonicHighPassHz", "Thick Harmonic High Pass", 25.0F, 200.0F, thick.harmonicHighPassHz);
    AddFloat(values, "thick.harmonicLowPassHz", "Thick Harmonic Low Pass", 4000.0F, 22000.0F, thick.harmonicLowPassHz);
    AddFloat(values, "thick.feedbackLoopGain", "Thick Feedback Loop Gain", 0.3F, 0.98F, thick.feedbackLoopGain);
    AddFloat(values, "thick.evenHarmonicWeight", "Thick Even Harmonic Weight", 0.0F, 0.3F, thick.evenHarmonicWeight);
    AddFloat(values, "thick.denseHarmonicGain", "Thick Dense Harmonic Gain", 0.0F, 0.5F, thick.denseHarmonicGain);
    AddFloat(values, "thick.denseHarmonicDecay", "Thick Dense Harmonic Decay", 0.75F, 1.0F, thick.denseHarmonicDecay);
    AddFloat(values, "thick.lowLevelDrive", "Thick Low Level Drive", 0.0F, 0.2F, thick.lowLevelDrive);
    AddFloat(values, "thick.richLevelDrive", "Thick Rich Level Drive", 0.0F, 2.0F, thick.richLevelDrive);
    AddFloat(values, "thick.subHarmonicCutoffHz",
        "Thick Sub Harmonic Cutoff", 40.0F, 300.0F,
        thick.subHarmonicCutoffHz);
    AddFloat(values, "thick.subHarmonicDrive", "Thick Sub Harmonic Drive", 0.0F, 1.0F, thick.subHarmonicDrive);
    AddFloat(values, "thick.transientCrestThreshold",
        "Thick Transient Crest Threshold", 1.0F, 4.0F,
        thick.transientCrestThreshold);
    AddFloat(values, "thick.transientMinimumGain",
        "Thick Transient Minimum Gain", 0.0F, 1.0F,
        thick.transientMinimumGain);
    AddFloat(values, "thick.transientBodyControl",
        "Thick Transient Body Control", 0.0F, 1.0F,
        thick.transientBodyControl);
#else
    PolishAirTuning air;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> values;
    AddFloat(values, "air", "Air", 0.0F, 1.0F, 0.0F);
    values.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("bypass", 1), "Bypass", false));
    AddFloat(values, "air.baselineHighPassHz", "Air Baseline High Pass", 1.0F, 30.0F, air.baselineHighPassHz);
    AddFloat(values, "air.directGain", "Air Direct Gain", -0.5F, 0.5F, air.directGain);
    AddFloat(values, "air.lowerShelfFrequencyHz", "Air Lower Shelf Frequency",
        50.0F, 12000.0F, air.lowerShelfFrequencyHz);
    AddFloat(values, "air.lowerShelfGain", "Air Lower Shelf Gain", -2.0F, 2.0F, air.lowerShelfGain);
    AddFloat(values, "air.middleShelfFrequencyHz", "Air Middle Shelf Frequency",
        500.0F, 16000.0F, air.middleShelfFrequencyHz);
    AddFloat(values, "air.middleShelfGain", "Air Middle Shelf Gain", -2.0F, 4.0F, air.middleShelfGain);
    AddFloat(values, "air.upperShelfFrequencyHz", "Air Upper Shelf Frequency",
        4000.0F, 22000.0F, air.upperShelfFrequencyHz);
    AddFloat(values, "air.upperShelfGain", "Air Upper Shelf Gain", -1.0F, 5.0F, air.upperShelfGain);
    AddFloat(values, "air.presencePeakFrequencyHz",
        "Air Presence Peak Frequency", 2000.0F, 10000.0F,
        air.presencePeakFrequencyHz);
    AddFloat(values, "air.presencePeakQ", "Air Presence Peak Q", 0.2F, 6.0F, air.presencePeakQ);
    AddFloat(values, "air.presencePeakGainDb", "Air Presence Peak Gain", -12.0F, 12.0F, air.presencePeakGainDb);
    AddFloat(values, "air.presenceNotchFrequencyHz",
        "Air Presence Notch Frequency", 3000.0F, 14000.0F,
        air.presenceNotchFrequencyHz);
    AddFloat(values, "air.presenceNotchQ", "Air Presence Notch Q", 0.2F, 8.0F, air.presenceNotchQ);
    AddFloat(values, "air.presenceNotchGainDb", "Air Presence Notch Gain", -18.0F, 6.0F, air.presenceNotchGainDb);
    AddFloat(values, "air.levelShelfFrequencyHz", "Air Level Shelf Frequency",
        3000.0F, 18000.0F, air.levelShelfFrequencyHz);
    AddFloat(values, "air.levelShelfGain", "Air Level Shelf Gain", -6.0F, 12.0F, air.levelShelfGain);
    AddFloat(values, "air.levelBellFrequencyHz", "Air Level Bell Frequency",
        3000.0F, 18000.0F, air.levelBellFrequencyHz);
    AddFloat(values, "air.levelBellQ", "Air Level Bell Q", 0.2F, 8.0F, air.levelBellQ);
    AddFloat(values, "air.levelBellGainDb", "Air Level Bell Gain", -24.0F, 6.0F, air.levelBellGainDb);
    AddFloat(values, "air.amountExponent", "Air Amount Exponent", 0.3F, 2.0F, air.amountExponent);
    AddFloat(values, "air.nonlinearSourceHighPassHz",
        "Air Nonlinear Source High Pass", 100.0F, 12000.0F,
        air.nonlinearSourceHighPassHz);
    AddFloat(values, "air.nonlinearOutputHighPassHz",
        "Air Nonlinear Output High Pass", 500.0F, 22000.0F,
        air.nonlinearOutputHighPassHz);
    AddFloat(values, "air.oddHarmonicGain", "Air Odd Harmonic Gain", 0.0F, 0.2F, air.oddHarmonicGain);
    AddFloat(values, "air.fifthOrderGain", "Air Fifth Order Gain", 0.0F, 0.05F, air.fifthOrderGain);
    AddFloat(values, "air.seventhOrderGain", "Air Seventh Order Gain", 0.0F, 0.02F, air.seventhOrderGain);
    AddFloat(values, "air.evenHarmonicHighPassHz",
        "Air Even Harmonic High Pass", 1000.0F, 22000.0F,
        air.evenHarmonicHighPassHz);
    AddFloat(values, "air.evenHarmonicGain", "Air Even Harmonic Gain", -0.02F, 0.02F, air.evenHarmonicGain);
    for (std::size_t index = 0; index < air.linearLevelGainAnchors.size(); ++index)
    {
        const auto id = "air.linearLevelGain" + std::to_string(index);
        const auto name = "Air Linear Level Gain " + std::to_string(index);
        AddFloat(values, id.c_str(), name.c_str(), 0.1F, 1.5F, air.linearLevelGainAnchors[index]);
    }
#endif
    return juce::AudioProcessorValueTreeState::ParameterLayout(
        values.begin(), values.end());
}

} // namespace

class PluginProcessor final : public juce::AudioProcessor
{
public:
    PluginProcessor()
        : AudioProcessor(BusesProperties()
            .withInput("Input", juce::AudioChannelSet::stereo(), true)
            .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
          parameters_(*this, nullptr, "Parameters", CreateParameterLayout())
    {
#if defined(CONSOLIDATOR_THICK_MODEL)
        amount_ = parameters_.getRawParameterValue("thick");
#else
        amount_ = parameters_.getRawParameterValue("air");
#endif
        bypass_ = parameters_.getRawParameterValue("bypass");
    }

    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override
    {
        const auto frameCapacity = static_cast<std::size_t>(
            std::max(maximumExpectedSamplesPerBlock, 1));
        left_.assign(frameCapacity, 0.0);
        right_.assign(frameCapacity, 0.0);
        processor_.SetTuningBeforePrepare(ReadTuning());
        processor_.Prepare({ sampleRate, frameCapacity });
        processor_.Reset();
        plan_ = {};
        plan_.channelMode = DspChannelMode::Stereo;
        policy_ = {};
        policy_.audible = true;
        policy_.polishActive = true;
    }

    void releaseResources() override
    {
        left_.clear();
        right_.clear();
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        juce::ScopedNoDenormals noDenormals;
        const auto frameCount = static_cast<std::size_t>(buffer.getNumSamples());
        if (buffer.getNumChannels() < 2 || frameCount > left_.size())
        {
            return;
        }
        const auto isBypassed = bypass_->load() >= 0.5F;
#if defined(CONSOLIDATOR_THICK_MODEL)
        snapshot_.polishThick = isBypassed ? 0.0F : amount_->load();
#else
        snapshot_.polishAir = isBypassed ? 0.0F : amount_->load();
#endif
        processor_.SetParameters(snapshot_);
        auto* left = buffer.getWritePointer(0);
        auto* right = buffer.getWritePointer(1);
        for (std::size_t index = 0; index < frameCount; ++index)
        {
            left_[index] = left[index];
            right_[index] = right[index];
        }
        DspAudioBuffer audio{ left_.data(), right_.data(), frameCount };
        DspProcessingFrame frame{ audio, {}, plan_, policy_ };
        processor_.Process(frame);
        for (std::size_t index = 0; index < frameCount; ++index)
        {
            left[index] = static_cast<float>(left_[index]);
            right[index] = static_cast<float>(right_[index]);
        }
    }

    juce::AudioProcessorEditor* createEditor() override
    {
        return new juce::GenericAudioProcessorEditor(*this);
    }

    bool hasEditor() const override { return true; }
    const juce::String getName() const override
    {
#if defined(CONSOLIDATOR_THICK_MODEL)
        return "Consolidator Thick Model";
#else
        return "Consolidator Air Model";
#endif
    }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    juce::AudioProcessorParameter* getBypassParameter() const override
    {
        return parameters_.getParameter("bypass");
    }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destination) override
    {
        if (auto xml = parameters_.copyState().createXml())
        {
            copyXmlToBinary(*xml, destination);
        }
    }

    void setStateInformation(const void* data, int sizeInBytes) override
    {
        if (const auto xml = getXmlFromBinary(data, sizeInBytes);
            xml != nullptr && xml->hasTagName(parameters_.state.getType()))
        {
            parameters_.replaceState(juce::ValueTree::fromXml(*xml));
        }
    }

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override
    {
        return layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo() &&
            layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
    }

    juce::AudioProcessorValueTreeState parameters_;

private:
    double Value(const char* id) const
    {
        return parameters_.getRawParameterValue(id)->load();
    }

#if defined(CONSOLIDATOR_THICK_MODEL)
    PolishThickTuning ReadTuning() const
    {
        PolishThickTuning value;
        value.bodyPeakFrequency = Value("thick.bodyPeakFrequency");
        value.bodyPeakGainDb = Value("thick.bodyPeakGainDb");
        value.bodyPeakQ = Value("thick.bodyPeakQ");
        value.subNotchFrequency = Value("thick.subNotchFrequency");
        value.subNotchGainDb = Value("thick.subNotchGainDb");
        value.subNotchQ = Value("thick.subNotchQ");
        value.positiveShoulderFrequency = Value("thick.positiveShoulderFrequency");
        value.positiveShoulderGainDb = Value("thick.positiveShoulderGainDb");
        value.positiveShoulderQ = Value("thick.positiveShoulderQ");
        value.positivePeakFrequency = Value("thick.positivePeakFrequency");
        value.positivePeakGainDb = Value("thick.positivePeakGainDb");
        value.positivePeakQ = Value("thick.positivePeakQ");
        for (std::size_t index = 0; index < value.bodyMixAnchors.size(); ++index)
        {
            value.bodyMixAnchors[index] = Value(
                ("thick.bodyMix" + std::to_string(index)).c_str());
        }
        value.sourceCutoffHz = Value("thick.sourceCutoffHz");
        value.harmonicHighPassHz = Value("thick.harmonicHighPassHz");
        value.harmonicLowPassHz = Value("thick.harmonicLowPassHz");
        value.feedbackLoopGain = Value("thick.feedbackLoopGain");
        value.evenHarmonicWeight = Value("thick.evenHarmonicWeight");
        value.denseHarmonicGain = Value("thick.denseHarmonicGain");
        value.denseHarmonicDecay = Value("thick.denseHarmonicDecay");
        value.lowLevelDrive = Value("thick.lowLevelDrive");
        value.richLevelDrive = Value("thick.richLevelDrive");
        value.subHarmonicCutoffHz = Value("thick.subHarmonicCutoffHz");
        value.subHarmonicDrive = Value("thick.subHarmonicDrive");
        value.transientCrestThreshold = Value("thick.transientCrestThreshold");
        value.transientMinimumGain = Value("thick.transientMinimumGain");
        value.transientBodyControl = Value("thick.transientBodyControl");
        return value;
    }

#else
    PolishAirTuning ReadTuning() const
    {
        PolishAirTuning value;
        value.baselineHighPassHz = Value("air.baselineHighPassHz");
        value.directGain = Value("air.directGain");
        value.lowerShelfFrequencyHz = Value("air.lowerShelfFrequencyHz");
        value.lowerShelfGain = Value("air.lowerShelfGain");
        value.middleShelfFrequencyHz = Value("air.middleShelfFrequencyHz");
        value.middleShelfGain = Value("air.middleShelfGain");
        value.upperShelfFrequencyHz = Value("air.upperShelfFrequencyHz");
        value.upperShelfGain = Value("air.upperShelfGain");
        value.presencePeakFrequencyHz = Value("air.presencePeakFrequencyHz");
        value.presencePeakQ = Value("air.presencePeakQ");
        value.presencePeakGainDb = Value("air.presencePeakGainDb");
        value.presenceNotchFrequencyHz = Value("air.presenceNotchFrequencyHz");
        value.presenceNotchQ = Value("air.presenceNotchQ");
        value.presenceNotchGainDb = Value("air.presenceNotchGainDb");
        value.levelShelfFrequencyHz = Value("air.levelShelfFrequencyHz");
        value.levelShelfGain = Value("air.levelShelfGain");
        value.levelBellFrequencyHz = Value("air.levelBellFrequencyHz");
        value.levelBellQ = Value("air.levelBellQ");
        value.levelBellGainDb = Value("air.levelBellGainDb");
        value.amountExponent = Value("air.amountExponent");
        value.nonlinearSourceHighPassHz = Value("air.nonlinearSourceHighPassHz");
        value.nonlinearOutputHighPassHz = Value("air.nonlinearOutputHighPassHz");
        value.oddHarmonicGain = Value("air.oddHarmonicGain");
        value.fifthOrderGain = Value("air.fifthOrderGain");
        value.seventhOrderGain = Value("air.seventhOrderGain");
        value.evenHarmonicHighPassHz = Value("air.evenHarmonicHighPassHz");
        value.evenHarmonicGain = Value("air.evenHarmonicGain");
        for (std::size_t index = 0; index < value.linearLevelGainAnchors.size(); ++index)
        {
            value.linearLevelGainAnchors[index] = Value(
                ("air.linearLevelGain" + std::to_string(index)).c_str());
        }
        return value;
    }
#endif

#if defined(CONSOLIDATOR_THICK_MODEL)
    PolishThickProcessor processor_;
#else
    PolishAirProcessor processor_;
#endif
    DspSnapshot snapshot_{};
    DspProcessingPlan plan_{};
    DspExecutionPolicy policy_{};
    std::vector<double> left_;
    std::vector<double> right_;
    std::atomic<float>* amount_{};
    std::atomic<float>* bypass_{};
};

} // namespace consolidator::model_plugins

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new consolidator::model_plugins::PluginProcessor();
}
