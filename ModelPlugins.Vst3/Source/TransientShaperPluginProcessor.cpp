#include <JuceHeader.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <memory>
#include <vector>

#include "TransientShaperProcessor.h"

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
    TransientShaperTuning tuning;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> values;
    AddFloat(values, "attack", "Attack", 0.0, 1.0, 0.0);
    AddFloat(values, "sustain", "Sustain", 0.0, 1.0, 0.0);
    values.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("bypass", 1), "Bypass", false));
    AddFloat(values, "detectorSeconds", "Detector Seconds",
        0.0005, 0.030, tuning.detectorSeconds);
    AddFloat(values, "fastEnvelopeSeconds", "Fast Envelope Seconds",
        0.0002, 0.020, tuning.fastEnvelopeSeconds);
    AddFloat(values, "slowEnvelopeSeconds", "Slow Envelope Seconds",
        0.005, 0.100, tuning.slowEnvelopeSeconds);
    AddFloat(values, "sustainEnvelopeSeconds", "Sustain Envelope Seconds",
        0.030, 0.500, tuning.sustainEnvelopeSeconds);
    AddFloat(values, "gainAttackSeconds", "Gain Attack Seconds",
        0.0001, 0.020, tuning.gainAttackSeconds);
    AddFloat(values, "gainReleaseSeconds", "Gain Release Seconds",
        0.005, 0.500, tuning.gainReleaseSeconds);
    AddFloat(values, "attackScale", "Attack Scale",
        0.1, 4.0, tuning.attackScale);
    AddFloat(values, "sustainScale", "Sustain Scale",
        0.1, 4.0, tuning.sustainScale);
    AddFloat(values, "maximumAttackDb", "Maximum Attack", 0.0, 18.0,
        tuning.maximumAttackDb);
    AddFloat(values, "maximumSustainDb", "Maximum Sustain", 0.0, 18.0,
        tuning.maximumSustainDb);
    AddFloat(values, "detectorGateDb", "Detector Gate", -120.0, -48.0,
        20.0 * std::log10(tuning.minimumDetectorLevel));
    AddFloat(values, "autoGainEnergySeconds", "Autogain Energy Seconds",
        0.050, 5.0, tuning.autoGain.energySeconds);
    AddFloat(values, "autoGainCorrectionSeconds", "Autogain Correction Seconds",
        0.050, 10.0, tuning.autoGain.correctionSeconds);
    AddFloat(values, "autoGainCompensationRatio", "Autogain Compensation Ratio",
        0.0, 1.0, tuning.autoGain.compensationRatio);
    AddFloat(values, "autoGainMaximumDb", "Autogain Maximum", 0.0, 12.0,
        tuning.autoGain.maximumCorrectionDb);
    AddFloat(values, "autoGainFloorDb", "Autogain Floor", -140.0, -60.0,
        10.0 * std::log10(tuning.autoGain.minimumInputEnergy));
    return juce::AudioProcessorValueTreeState::ParameterLayout(
        values.begin(), values.end());
}

} // namespace

class TransientShaperPluginProcessor final : public juce::AudioProcessor
{
public:
    TransientShaperPluginProcessor()
        : AudioProcessor(BusesProperties()
            .withInput("Input", juce::AudioChannelSet::stereo(), true)
            .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
          parameters_(*this, nullptr, "Parameters", CreateParameterLayout())
    {
        attack_ = parameters_.getRawParameterValue("attack");
        sustain_ = parameters_.getRawParameterValue("sustain");
        bypass_ = parameters_.getRawParameterValue("bypass");
    }

    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override
    {
        const auto capacity = static_cast<std::size_t>(
            std::max(maximumExpectedSamplesPerBlock, 1));
        left_.assign(capacity, 0.0);
        right_.assign(capacity, 0.0);
        processor_.SetTuningBeforePrepare(ReadTuning());
        processor_.Prepare({ sampleRate, capacity });
        processor_.Reset();
        plan_ = {};
        policy_ = {};
        policy_.audible = true;
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
        const auto bypassed = bypass_->load() >= 0.5F;
        snapshot_.compressorAttack = bypassed ? 0.0F : attack_->load();
        snapshot_.compressorSustain = bypassed ? 0.0F : sustain_->load();
        processor_.SetParameters(snapshot_);
        auto* left = buffer.getWritePointer(0);
        auto* right = buffer.getWritePointer(1);
        for (std::size_t index = 0; index < frameCount; ++index)
        {
            left_[index] = left[index];
            right_[index] = right[index];
        }
        DspAudioBuffer audio{ left_.data(), right_.data(), frameCount };
        const DspDetectorBuffer detector{ left_.data(), right_.data() };
        DspProcessingFrame frame{ audio, detector, plan_, policy_ };
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
        return "Consolidator Transient Shaper Model";
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

    TransientShaperTuning ReadTuning() const
    {
        TransientShaperTuning tuning;
        tuning.detectorSeconds = Value("detectorSeconds");
        tuning.fastEnvelopeSeconds = Value("fastEnvelopeSeconds");
        tuning.slowEnvelopeSeconds = Value("slowEnvelopeSeconds");
        tuning.sustainEnvelopeSeconds = Value("sustainEnvelopeSeconds");
        tuning.gainAttackSeconds = Value("gainAttackSeconds");
        tuning.gainReleaseSeconds = Value("gainReleaseSeconds");
        tuning.attackScale = Value("attackScale");
        tuning.sustainScale = Value("sustainScale");
        tuning.maximumAttackDb = Value("maximumAttackDb");
        tuning.maximumSustainDb = Value("maximumSustainDb");
        tuning.minimumDetectorLevel = std::pow(10.0, Value("detectorGateDb") / 20.0);
        tuning.autoGain.energySeconds = Value("autoGainEnergySeconds");
        tuning.autoGain.correctionSeconds = Value("autoGainCorrectionSeconds");
        tuning.autoGain.compensationRatio = Value("autoGainCompensationRatio");
        tuning.autoGain.maximumCorrectionDb = Value("autoGainMaximumDb");
        tuning.autoGain.minimumInputEnergy = std::pow(
            10.0, Value("autoGainFloorDb") / 10.0);
        return tuning;
    }

    TransientShaperProcessor processor_;
    DspSnapshot snapshot_{};
    DspProcessingPlan plan_{};
    DspExecutionPolicy policy_{};
    std::vector<double> left_;
    std::vector<double> right_;
    std::atomic<float>* attack_{};
    std::atomic<float>* sustain_{};
    std::atomic<float>* bypass_{};
};

} // namespace consolidator::model_plugins

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new consolidator::model_plugins::TransientShaperPluginProcessor();
}
