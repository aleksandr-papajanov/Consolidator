#pragma once

#include <atomic>
#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include <JuceHeader.h>

#include "Audio/Runtime/AudioRuntime.h"
#include "Transport/ManagedBridge.h"
#include "Shared/Protocol/ProtocolFrame.h"

namespace consolidator::vst3::host::plugin
{

class PluginProcessor final : public juce::AudioProcessor, private juce::Timer
{
public:
    PluginProcessor();
    ~PluginProcessor() override;

    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override;
    void reset() override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override;
    void processBlock(juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midi) override;
    bool supportsDoublePrecisionProcessing() const override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& name) override;
    void getStateInformation(juce::MemoryBlock& destination) override;
    void setStateInformation(const void* data, int sizeInBytes) override;
    void updateTrackProperties(const TrackProperties& properties) override;

    [[nodiscard]] std::uint64_t InstanceId() const noexcept;
    [[nodiscard]] bool IsReady() const noexcept;
    [[nodiscard]] double PresentationSampleRate() const noexcept;
    // Message-thread only. The editor removes its listener before destruction.
    void SetOutputListener(std::function<void(const shared::protocol::ProtocolFrame&)> listener);
    // Control commands are encoded only on the message thread, never from processBlock.
    [[nodiscard]] std::uint64_t Request(const std::string& selector, std::vector<shared::protocol::ProtocolValue> body = {});

private:
    static void __cdecl ReceiveOutput(
        void* context,
        const char* selector,
        const ::consolidator::transport::NativeAtom* atoms,
        std::size_t count) noexcept;
    void QueueOutput(
        const char* selector,
        const ::consolidator::transport::NativeAtom* atoms,
        std::size_t count);
    void timerCallback() override;
    void Process(double* left, double* right, const double* referenceLeft,
        const double* referenceRight, std::size_t count) noexcept;

    ::consolidator::transport::ManagedBridge managed_;
    ::consolidator::transport::DspStateExchange exchange_{};
    audio::runtime::AudioRuntime runtime_;
    std::uint64_t instanceId_{};
    ::consolidator::transport::AudioInputHandle audioHandle_{};
    std::vector<double> left_;
    std::vector<double> right_;
    std::vector<double> referenceLeft_;
    std::vector<double> referenceRight_;
    std::uint64_t nextRequestId_{ 1 };
    std::mutex outputMutex_;
    std::deque<shared::protocol::ProtocolFrame> controlFrames_;
    std::optional<shared::protocol::ProtocolFrame> fftFrame_;
    std::optional<shared::protocol::ProtocolFrame> telemetryFrame_;
    std::optional<shared::protocol::ProtocolFrame> trackLevelsFrame_;
    std::function<void(const shared::protocol::ProtocolFrame&)> listener_;
    std::atomic_bool editorAttached_{};
    std::atomic_bool persistenceDirty_{};
    std::atomic_bool stateRestored_{};
    std::atomic_uint64_t failedCallbacks_{};
    std::atomic<double> presentationSampleRate_{};
    double deliveredSampleRate_{ -1 };
    bool prepared_{};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginProcessor)
};

} // namespace consolidator::vst3::host::plugin
