#include <JuceHeader.h>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <memory>
#include <cmath>
#include <vector>

#if defined(CONSOLIDATOR_THICK_MODEL)
#include "Processors/Thick/PolishThickProcessor.h"
#elif defined(CONSOLIDATOR_AIR_MODEL)
#include "Processors/Air/PolishAirProcessor.h"
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
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> values;
#if defined(CONSOLIDATOR_THICK_MODEL)
    AddFloat(values, "thick", "Thick", 0.0, 1.0, 0.0);
#else
    AddFloat(values, "air", "Air", 0.0, 1.0, 0.0);
#endif
    values.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("bypass", 1), "Bypass", false));
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
#if defined(CONSOLIDATOR_THICK_MODEL)
    processors::PolishThickProcessor processor_;
#else
    processors::PolishAirProcessor processor_;
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
