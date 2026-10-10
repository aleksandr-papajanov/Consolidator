#include <algorithm>
#include <atomic>
#include <cstddef>
#include <memory>
#include <vector>

#include <JuceHeader.h>

#include "TransientShaperProcessor.h"

namespace consolidator::model_plugins
{

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
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> values;
    AddFloat(values, "attack", "Attack", -1.0, 1.0, 0.0);
    AddFloat(values, "sustain", "Sustain", -1.0, 1.0, 0.0);
    values.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("bypass", 1), "Bypass", false));
    max::dsp::TransientShaperTuning tuning;
    AddFloat(values, "maskSidechainHz", "Mask Sidechain Hz", 20, 2000,
        tuning.sidechainHz);
    AddFloat(values, "maskFastAttackSeconds", "Mask Tail Fast Power Attack Seconds", 0.00005, 0.005,
        tuning.fastAttackSeconds);
    AddFloat(values, "maskFastReleaseSeconds", "Mask Tail Fast Power Release Seconds", 0.003, 0.1,
        tuning.fastReleaseSeconds);
    AddFloat(values, "maskSlowAttackSeconds", "Mask Tail Slow Power Attack Seconds", 0.0005, 0.05,
        tuning.slowAttackSeconds);
    AddFloat(values, "maskSlowReleaseSeconds", "Mask Tail Slow Power Release Seconds", 0.05, 2,
        tuning.slowReleaseSeconds);
    AddFloat(values, "maskContextSeconds", "Mask Onset Context Seconds", 0.002, 0.1,
        tuning.contextSeconds);
    AddFloat(values, "maskRiseDb", "Mask Relative Rise dB", 1.5, 18,
        tuning.riseDb);
    AddFloat(values, "maskAttackReleaseSeconds", "Mask Attack Release Seconds", 0.001, 0.15,
        tuning.attackReleaseSeconds);
    AddFloat(values, "maskCloseSeconds", "Mask Sustain Close Seconds", 0.00001, 0.02,
        tuning.maskCloseSeconds);
    AddFloat(values, "maskSustainOpenSeconds", "Mask Sustain Open Seconds", 0.00001, 0.1,
        tuning.sustainOpenSeconds);
    AddFloat(values, "maskPeakReleaseSeconds", "Mask Peak Release Seconds", 0.03, 2,
        tuning.peakReleaseSeconds);
    AddFloat(values, "maskActivityExponent", "Mask Activity Exponent", 0.0001, 0.5,
        tuning.activityExponent);
    AddFloat(values, "maskTailBoost", "Mask Tail Boost", 0.2, 3,
        tuning.tailBoost);
    AddFloat(values, "maskTailExponent", "Mask Tail Exponent", 0.1, 1,
        tuning.tailExponent);
    AddFloat(values, "maskBaseGainDb", "Mask Base Gain dB", -12, 0,
        tuning.baseGainDb);
    AddFloat(values, "maskSteadyReturn", "Mask Steady Return", 0, 1,
        tuning.steadyReturn);
    AddFloat(values, "maskAttackBoost", "Mask Attack Boost", 0, 16,
        tuning.attackBoost);
    AddFloat(values, "maskStrengthRangeDb", "Mask Strength Range dB", 40, 160,
        tuning.strengthRangeDb);
    AddFloat(values, "maskInitialSlowLevelDb", "Mask Initial Slow Level dB", -100, -20,
        tuning.initialSlowLevelDb);
    AddFloat(values, "outputGainDb", "Output Gain dB", -12, 12, 0);
    AddFloat(values, "maskAttackAdditionReleaseSeconds", "Mask Attack Addition Release Seconds", 0.02, 0.15,
        tuning.attackAdditionReleaseSeconds);
    AddFloat(values, "maskAttackAdditionMix", "Mask Attack Addition Mix", 0, 1,
        tuning.attackAdditionMix);
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
        outputGainDb_ = parameters_.getRawParameterValue("outputGainDb");
    }

    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override
    {
        const auto capacity = static_cast<std::size_t>(
            std::max(maximumExpectedSamplesPerBlock, 1));
        left_.assign(capacity, 0.0);
        right_.assign(capacity, 0.0);
        transientShaper_.SetTuningBeforePrepare(ReadMaskTuning());
        transientShaper_.SetOutputGainDb(outputGainDb_->load());
        transientShaper_.Prepare({ sampleRate, capacity });
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
        if (bypassed)
        {
            return;
        }
        transientShaper_.SetAmounts(attack_->load(), sustain_->load());
        transientShaper_.SetOutputGainDb(outputGainDb_->load());
        auto* left = buffer.getWritePointer(0);
        auto* right = buffer.getWritePointer(1);
        for (std::size_t index = 0; index < frameCount; ++index)
        {
            left_[index] = left[index];
            right_[index] = right[index];
        }
        transientShaper_.Process({ left_.data(), right_.data(), frameCount });
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

    max::dsp::TransientShaperTuning ReadMaskTuning() const
    {
        max::dsp::TransientShaperTuning tuning;
        tuning.sidechainHz = Value("maskSidechainHz");
        tuning.fastAttackSeconds = Value("maskFastAttackSeconds");
        tuning.fastReleaseSeconds = Value("maskFastReleaseSeconds");
        tuning.slowAttackSeconds = Value("maskSlowAttackSeconds");
        tuning.slowReleaseSeconds = Value("maskSlowReleaseSeconds");
        tuning.contextSeconds = Value("maskContextSeconds");
        tuning.riseDb = Value("maskRiseDb");
        tuning.attackReleaseSeconds = Value("maskAttackReleaseSeconds");
        tuning.attackAdditionReleaseSeconds = Value("maskAttackAdditionReleaseSeconds");
        tuning.attackAdditionMix = Value("maskAttackAdditionMix");
        tuning.maskCloseSeconds = Value("maskCloseSeconds");
        tuning.sustainOpenSeconds = Value("maskSustainOpenSeconds");
        tuning.peakReleaseSeconds = Value("maskPeakReleaseSeconds");
        tuning.activityExponent = Value("maskActivityExponent");
        tuning.tailBoost = Value("maskTailBoost");
        tuning.tailExponent = Value("maskTailExponent");
        tuning.baseGainDb = Value("maskBaseGainDb");
        tuning.steadyReturn = Value("maskSteadyReturn");
        tuning.attackBoost = Value("maskAttackBoost");
        tuning.strengthRangeDb = Value("maskStrengthRangeDb");
        tuning.initialSlowLevelDb = Value("maskInitialSlowLevelDb");
        return tuning;
    }

    max::dsp::TransientShaperProcessor transientShaper_;
    std::vector<double> left_;
    std::vector<double> right_;
    std::atomic<float>* attack_{};
    std::atomic<float>* sustain_{};
    std::atomic<float>* bypass_{};
    std::atomic<float>* outputGainDb_{};
};

} // namespace consolidator::model_plugins

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new consolidator::model_plugins::TransientShaperPluginProcessor();
}
