#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cmath>
#include <memory>
#include <vector>

#include <JuceHeader.h>

#include "Processors/TransientShaper/TransientShaperProcessor.h"

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
    AddFloat(values, "outputGainDb", "Output Gain dB", -12, 12, 0);
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
            const auto saved = juce::ValueTree::fromXml(*xml);
            auto state = parameters_.copyState();
            for (const auto child : state)
            {
                const auto id = child.getProperty("id").toString();
                const auto control = saved.getChildWithProperty("id", id);
                if (!control.isValid() || !control.hasProperty("value") ||
                    !std::isfinite(static_cast<float>(control.getProperty("value"))))
                {
                    juce::Logger::writeToLog(getName() + " state is missing a valid user control: " + id);
                    return;
                }
            }
            for (auto child : state)
            {
                const auto id = child.getProperty("id").toString();
                child.setProperty("value", saved.getChildWithProperty("id", id).getProperty("value"), nullptr);
            }
            parameters_.replaceState(state);
        }
    }

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override
    {
        return layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo() &&
            layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
    }

    juce::AudioProcessorValueTreeState parameters_;

private:
    max::dsp::processors::TransientShaperProcessor transientShaper_;
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
