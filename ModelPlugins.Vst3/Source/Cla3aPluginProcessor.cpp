#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cmath>
#include <memory>
#include <vector>

#include <JuceHeader.h>

#include "Processors/Cla3a/Cla3aProcessor.h"

namespace consolidator::model_plugins
{

namespace
{

void AddAttenuation(
    std::vector<std::unique_ptr<juce::RangedAudioParameter>>& parameters,
    const char* id,
    const char* name,
    double defaultValue)
{
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(id, 1), name,
        juce::NormalisableRange<float>(0.0F, 1.0F, 0.0F, 0.3F),
        static_cast<float>(defaultValue),
        juce::AudioParameterFloatAttributes()
            .withLabel("dB")
            .withStringFromValueFunction([](float gain, int)
            {
                return gain == 0.0F ? juce::String("-inf")
                    : juce::String(20.0 * std::log10(gain), 2);
            })
            .withValueFromStringFunction([](const juce::String& text)
            {
                return text.containsIgnoreCase("inf") ? 0.0F
                    : static_cast<float>(std::pow(10.0, text.getDoubleValue() / 20.0));
            })));
}

juce::AudioProcessorValueTreeState::ParameterLayout CreateParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> values;
    const max::dsp::processors::Cla3aControls defaults;
    values.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("reduction", 1), "Reduction", juce::NormalisableRange<float>(0.0F, 1.0F),
        static_cast<float>(defaults.reduction)));
    AddAttenuation(values, "output", "Output", defaults.outputGain);
    values.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("bypass", 1), "Bypass", false));
    return juce::AudioProcessorValueTreeState::ParameterLayout(
        values.begin(), values.end());
}

} // namespace

class Cla3aPluginProcessor final : public juce::AudioProcessor
{
public:
    Cla3aPluginProcessor()
        : AudioProcessor(BusesProperties()
            .withInput("Input", juce::AudioChannelSet::stereo(), true)
            .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
          parameters_(*this, nullptr, "Parameters", CreateParameterLayout())
    {
        reduction_ = parameters_.getRawParameterValue("reduction");
        output_ = parameters_.getRawParameterValue("output");
        bypass_ = parameters_.getRawParameterValue("bypass");
    }

    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override
    {
        const auto capacity = static_cast<std::size_t>(
            std::max(maximumExpectedSamplesPerBlock, 1));
        left_.assign(capacity, 0.0);
        right_.assign(capacity, 0.0);
        processor_.SetControls(reduction_->load(), output_->load());
        processor_.Prepare({ sampleRate, capacity });
    }

    void releaseResources() override
    {
        left_.clear();
        right_.clear();
    }

    void reset() override
    {
        processor_.Reset();
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
        processor_.SetControls(reduction_->load(), output_->load());
        auto* left = buffer.getWritePointer(0);
        auto* right = buffer.getWritePointer(1);
        for (std::size_t index = 0; index < frameCount; ++index)
        {
            left_[index] = left[index];
            right_[index] = right[index];
        }
        processor_.Process({ left_.data(), right_.data(), frameCount });
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
        return "Consolidator CLA-3A Model";
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
        const auto state = parameters_.copyState();
        if (auto xml = state.createXml())
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
                    juce::Logger::writeToLog("CLA-3A state is missing a valid user control: " + id);
                    return;
                }
            }
            for (auto child : state)
            {
                const auto id = child.getProperty("id").toString();
                const auto value = saved.getChildWithProperty("id", id).getProperty("value");
                child.setProperty("value", value, nullptr);
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
    max::dsp::processors::Cla3aProcessor processor_;
    std::vector<double> left_;
    std::vector<double> right_;
    std::atomic<float>* reduction_{};
    std::atomic<float>* output_{};
    std::atomic<float>* bypass_{};
};

} // namespace consolidator::model_plugins

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new consolidator::model_plugins::Cla3aPluginProcessor();
}

