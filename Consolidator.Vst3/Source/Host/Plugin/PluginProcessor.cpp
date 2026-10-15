#include "Host/Plugin/PluginProcessor.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <exception>
#include <utility>

#include "Host/Plugin/PluginEditor.h"
#include "Host/Plugin/WindowsPluginHost.h"

namespace consolidator::vst3::host::plugin
{

PluginProcessor::PluginProcessor()
    : AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withInput("Sidechain", juce::AudioChannelSet::stereo(), false)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      managed_(ResolveManagedLibraryPath(), &WriteManagedLog)
{
    if (managed_.IsLoaded())
    {
        instanceId_ = managed_.RegisterInstance(this, &ReceiveOutput, &exchange_, &audioHandle_);
    }
    if (instanceId_ == 0)
    {
        WriteManagedLog(nullptr, "VST3 instance registration failed; audio is disabled.");
    }
    startTimerHz(30);
}

PluginProcessor::~PluginProcessor()
{
    // The host must stop processBlock before destroying its AudioProcessor.
    const juce::MessageManagerLock messageLock;
    stopTimer();
    editorAttached_.store(false);
    listener_ = {};
    if (instanceId_ != 0)
    {
        managed_.UnregisterInstance(instanceId_);
    }
    instanceId_ = 0;
    audioHandle_ = 0;
    // Unregister's callback barrier completes before queue/exchange destruction.
}

void PluginProcessor::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock)
{
    prepared_ = false;
    presentationSampleRate_.store(0);
    if (!IsReady() || !std::isfinite(sampleRate) || sampleRate <= 0.0)
    {
        WriteManagedLog(nullptr, "Cannot prepare VST3 audio without a registered Managed instance and sample rate.");
        return;
    }
    const auto capacity = static_cast<std::size_t>(std::max(1, maximumExpectedSamplesPerBlock));
    left_.assign(capacity, 0.0);
    right_.assign(capacity, 0.0);
    referenceLeft_.assign(capacity, 0.0);
    referenceRight_.assign(capacity, 0.0);
    runtime_.Prepare(sampleRate, capacity);
    runtime_.Reset();
    managed_.Prepare(instanceId_, sampleRate, capacity);
    prepared_ = true;
    presentationSampleRate_.store(sampleRate);
}

double PluginProcessor::PresentationSampleRate() const noexcept
{
    return presentationSampleRate_.load();
}

void PluginProcessor::releaseResources()
{
    prepared_ = false;
    presentationSampleRate_.store(0);
    runtime_.Reset();
    left_.clear();
    right_.clear();
    referenceLeft_.clear();
    referenceRight_.clear();
}

void PluginProcessor::reset()
{
    runtime_.Reset();
}

void PluginProcessor::Process(double* left, double* right, const double* referenceLeft,
    const double* referenceRight, std::size_t count) noexcept
{
    // Managed owns analysis of both dry input pairs; DSP consumes only main audio.
    managed_.SendAudio(audioHandle_, left, right, referenceLeft, referenceRight, count);
    runtime_.ProcessBlock(exchange_, left, right, count);
}

void PluginProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    if (!prepared_)
    {
        buffer.clear();
        return;
    }
    const auto count = static_cast<std::size_t>(buffer.getNumSamples());
    const auto reference = getBusBuffer(buffer, true, 1);
    const bool hasReference = reference.getNumChannels() == 2;
    auto main = getBusBuffer(buffer, false, 0);
    auto* left = main.getWritePointer(0);
    auto* right = main.getWritePointer(1);
    for (std::size_t offset = 0; offset < count;)
    {
        const auto frames = std::min(left_.size(), count - offset);
        for (std::size_t index = 0; index < frames; ++index)
        {
            left_[index] = left[offset + index];
            right_[index] = right[offset + index];
            if (hasReference)
            {
                referenceLeft_[index] = reference.getReadPointer(0)[offset + index];
                referenceRight_[index] = reference.getReadPointer(1)[offset + index];
            }
        }
        Process(left_.data(), right_.data(), hasReference ? referenceLeft_.data() : nullptr,
            hasReference ? referenceRight_.data() : nullptr, frames);
        for (std::size_t index = 0; index < frames; ++index)
        {
            left[offset + index] = static_cast<float>(left_[index]);
            right[offset + index] = static_cast<float>(right_[index]);
        }
        offset += frames;
    }
}

void PluginProcessor::processBlock(juce::AudioBuffer<double>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    if (!prepared_)
    {
        buffer.clear();
        return;
    }
    const auto count = static_cast<std::size_t>(buffer.getNumSamples());
    const auto reference = getBusBuffer(buffer, true, 1);
    const bool hasReference = reference.getNumChannels() == 2;
    auto main = getBusBuffer(buffer, false, 0);
    for (std::size_t offset = 0; offset < count;)
    {
        const auto frames = std::min(left_.size(), count - offset);
        Process(main.getWritePointer(0) + offset, main.getWritePointer(1) + offset,
            hasReference ? reference.getReadPointer(0) + offset : nullptr,
            hasReference ? reference.getReadPointer(1) + offset : nullptr, frames);
        offset += frames;
    }
}

bool PluginProcessor::supportsDoublePrecisionProcessing() const { return true; }
bool PluginProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.inputBuses.size() == 2 && layouts.outputBuses.size() == 1 &&
        layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo() &&
        (layouts.inputBuses[1].isDisabled() || layouts.inputBuses[1] == juce::AudioChannelSet::stereo()) &&
        layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}
juce::AudioProcessorEditor* PluginProcessor::createEditor() { return new PluginEditor(*this); }
bool PluginProcessor::hasEditor() const { return true; }
const juce::String PluginProcessor::getName() const { return "Consolidator"; }
bool PluginProcessor::acceptsMidi() const { return false; }
bool PluginProcessor::producesMidi() const { return false; }
bool PluginProcessor::isMidiEffect() const { return false; }
double PluginProcessor::getTailLengthSeconds() const { return 0.0; }
int PluginProcessor::getNumPrograms() { return 1; }
int PluginProcessor::getCurrentProgram() { return 0; }
void PluginProcessor::setCurrentProgram(int) {}
const juce::String PluginProcessor::getProgramName(int) { return {}; }
void PluginProcessor::changeProgramName(int, const juce::String&) {}

void PluginProcessor::getStateInformation(juce::MemoryBlock& destination)
{
    destination.reset();
    if (!IsReady())
    {
        WriteManagedLog(nullptr, "Cannot capture state for an unregistered VST3 instance.");
        return;
    }
    const auto snapshot = managed_.CapturePersistence(instanceId_);
    if (snapshot.empty())
    {
        WriteManagedLog(nullptr, "Managed persistence capture failed.");
        return;
    }
    destination.replaceAll(snapshot.data(), snapshot.size());
}

void PluginProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (!IsReady() || data == nullptr || sizeInBytes <= 0 ||
        !managed_.RestorePersistence(instanceId_, static_cast<const char*>(data),
            static_cast<std::size_t>(sizeInBytes)))
    {
        WriteManagedLog(nullptr, "Managed persistence restore rejected the VST3 state.");
        return;
    }
    // Managed suppresses dirty notifications for its restored baseline.
    stateRestored_.store(true, std::memory_order_release);
}

void PluginProcessor::updateTrackProperties(const TrackProperties& properties)
{
    if (IsReady() && properties.name.isNotEmpty())
    {
        const auto request = Request("write", { std::string("local"), std::string("0"), std::int64_t{ 1 },
            std::string("entry"), std::string("label"), std::string("value"),
            properties.name.toStdString(), std::string("copy") });
        juce::ignoreUnused(request);
    }
}

std::uint64_t PluginProcessor::InstanceId() const noexcept { return instanceId_; }
bool PluginProcessor::IsReady() const noexcept { return instanceId_ != 0; }

void PluginProcessor::SetOutputListener(std::function<void(const shared::protocol::ProtocolFrame&)> listener)
{
    editorAttached_.store(false);
    {
        const std::lock_guard lock(outputMutex_);
        controlFrames_.clear();
        fftFrame_.reset();
        telemetryFrame_.reset();
        trackLevelsFrame_.reset();
        listener_ = std::move(listener);
        deliveredSampleRate_ = -1;
        editorAttached_.store(static_cast<bool>(listener_));
    }
}

std::uint64_t PluginProcessor::Request(const std::string& selector, std::vector<shared::protocol::ProtocolValue> body)
{
    if (!IsReady())
    {
        WriteManagedLog(nullptr, ("Cannot submit command without a registered Managed instance: " + selector).c_str());
        return 0;
    }
    const auto requestId = nextRequestId_++;
    std::vector<shared::protocol::ProtocolValue> values{ std::int64_t{ 1 }, std::string("juce"), std::to_string(requestId) };
    values.insert(values.end(), body.begin(), body.end());
    const auto atoms = shared::protocol::EncodeAtoms(values);
    managed_.SendManagedMessage(instanceId_, selector.c_str(), atoms.data(), atoms.size());
    return requestId;
}

void __cdecl PluginProcessor::ReceiveOutput(
    void* context, const char* selector, const max::transport::NativeAtom* atoms, std::size_t count) noexcept
{
    if (context == nullptr)
    {
        return;
    }
    auto& processor = *static_cast<PluginProcessor*>(context);
    try
    {
        processor.QueueOutput(selector, atoms, count);
    }
    catch (...)
    {
        processor.failedCallbacks_.fetch_add(1, std::memory_order_relaxed);
    }
}

void PluginProcessor::QueueOutput(
    const char* selector, const max::transport::NativeAtom* atoms, std::size_t count)
{
    if (selector != nullptr && std::strcmp(selector, "persistence_dirty") == 0)
    {
        persistenceDirty_.store(true, std::memory_order_release);
        return;
    }
    if (!editorAttached_.load() && (selector == nullptr || std::strcmp(selector, "error") != 0))
    {
        return;
    }
    auto frame = shared::protocol::CopyFrame(selector, atoms, count);
    if (frame.selector == "error")
    {
        if (frame.values.size() >= 4)
        {
            WriteManagedLog(nullptr, shared::protocol::Symbol(frame.values[3]).c_str());
        }
    }
    if (!editorAttached_.load())
    {
        return;
    }
    const std::lock_guard lock(outputMutex_);
    if (!editorAttached_.load())
    {
        return;
    }
    if (frame.selector == "fft")
    {
        fftFrame_ = std::move(frame);
    }
    else if (frame.selector == "dsp_telemetry")
    {
        telemetryFrame_ = std::move(frame);
    }
    else if (frame.selector == "track_levels")
    {
        trackLevelsFrame_ = std::move(frame);
    }
    else
    {
        controlFrames_.push_back(std::move(frame));
    }
}

void PluginProcessor::timerCallback()
{
    if (persistenceDirty_.exchange(false, std::memory_order_acq_rel))
    {
        updateHostDisplay(ChangeDetails{}.withNonParameterStateChanged(true));
    }
    const auto failures = failedCallbacks_.exchange(0);
    if (failures != 0)
    {
        juce::Logger::writeToLog("Consolidator callback failures: " + juce::String(failures));
    }

    std::vector<shared::protocol::ProtocolFrame> frames;
    {
        const std::lock_guard lock(outputMutex_);
        // Limit control work per message-thread tick; preserve FIFO ordering.
        for (int index = 0; index < 32 && !controlFrames_.empty(); ++index)
        {
            frames.push_back(std::move(controlFrames_.front()));
            controlFrames_.pop_front();
        }
        for (auto* latest : { &fftFrame_, &telemetryFrame_, &trackLevelsFrame_ })
        {
            if (*latest)
            {
                frames.push_back(std::move(**latest));
                latest->reset();
            }
        }
    }
    if (stateRestored_.exchange(false, std::memory_order_acq_rel) && listener_)
    {
        listener_({ "state_restored", {} });
    }
    const auto sampleRate = PresentationSampleRate();
    if (listener_ && deliveredSampleRate_ != sampleRate)
    {
        deliveredSampleRate_ = sampleRate;
        listener_({ "audio_configuration_changed", { sampleRate } });
    }
    for (const auto& frame : frames)
    {
        if (listener_)
        {
            try
            {
                listener_(frame);
            }
            catch (const std::exception& exception)
            {
                WriteManagedLog(nullptr, exception.what());
            }
        }
    }
}

} // namespace consolidator::vst3::host::plugin

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new consolidator::vst3::host::plugin::PluginProcessor();
}
