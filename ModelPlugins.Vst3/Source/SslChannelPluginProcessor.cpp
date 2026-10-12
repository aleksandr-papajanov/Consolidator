#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cmath>
#include <memory>
#include <vector>

#include <JuceHeader.h>

#include "Processors/SslChannel/SslChannelProcessor.h"

namespace consolidator::model_plugins
{

namespace
{

juce::AudioProcessorValueTreeState::ParameterLayout CreateParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> values;
    const max::dsp::processors::SslChannelControls defaults;
    values.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("ratio", 1), "Ratio", juce::NormalisableRange<float>(1.0F, 100.0F, 0.0F, 0.3F),
        static_cast<float>(defaults.ratio), juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction([](float value, int)
            {
                return value >= 100.0F ? juce::String("Inf") : juce::String(value, 2);
            })
            .withValueFromStringFunction([](const juce::String& text)
            {
                return text.containsIgnoreCase("inf") ? 100.0F : text.getFloatValue();
            })));
    values.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("release", 1), "Release", juce::NormalisableRange<float>(0.1F, 4.0F, 0.0F, 0.3F),
        static_cast<float>(defaults.releaseSeconds), juce::AudioParameterFloatAttributes().withLabel("s")));
    values.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("fastAttack", 1), "Fast Attack", defaults.fastAttack));
    values.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("threshold", 1), "Threshold", juce::NormalisableRange<float>(-20.0F, 10.0F),
        static_cast<float>(defaults.thresholdDb), juce::AudioParameterFloatAttributes().withLabel("dBu")));
    values.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("bypass", 1), "Bypass", false));
    return juce::AudioProcessorValueTreeState::ParameterLayout(values.begin(), values.end());
}

} // namespace

class SslChannelPluginProcessor final : public juce::AudioProcessor
{
public:
    SslChannelPluginProcessor()
        : AudioProcessor(BusesProperties()
            .withInput("Input", juce::AudioChannelSet::stereo(), true)
            .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
          parameters_(*this, nullptr, "Parameters", CreateParameterLayout())
    {
        ratio_ = parameters_.getRawParameterValue("ratio");
        release_ = parameters_.getRawParameterValue("release");
        fastAttack_ = parameters_.getRawParameterValue("fastAttack");
        threshold_ = parameters_.getRawParameterValue("threshold");
        bypass_ = parameters_.getRawParameterValue("bypass");
    }

    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override
    {
        const auto capacity = static_cast<std::size_t>(
            std::max(maximumExpectedSamplesPerBlock, 1));
        left_.assign(capacity, 0.0);
        right_.assign(capacity, 0.0);
        processor_.SetControls(ratio_->load(), release_->load(), fastAttack_->load() >= 0.5F, threshold_->load());
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
        processor_.SetControls(ratio_->load(), release_->load(), fastAttack_->load() >= 0.5F, threshold_->load());
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
        return "Consolidator SSLChannel Model";
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
                    juce::Logger::writeToLog("SSLChannel state is missing a valid user control: " + id);
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
    max::dsp::processors::SslChannelProcessor processor_;
    std::vector<double> left_;
    std::vector<double> right_;
    std::atomic<float>* ratio_{};
    std::atomic<float>* release_{};
    std::atomic<float>* fastAttack_{};
    std::atomic<float>* threshold_{};
    std::atomic<float>* bypass_{};
};

} // namespace consolidator::model_plugins

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new consolidator::model_plugins::SslChannelPluginProcessor();
}
