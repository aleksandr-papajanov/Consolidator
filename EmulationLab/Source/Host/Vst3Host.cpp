#include "Vst3Host.h"

#include <algorithm>
#include <stdexcept>

namespace emulation_lab
{

Vst3Plugin::Vst3Plugin(
    std::unique_ptr<juce::AudioPluginInstance> instance,
    double sampleRate,
    int blockSize)
    : instance_(std::move(instance)),
      sampleRate_(sampleRate),
      blockSize_(blockSize),
      buffer_(2, blockSize)
{
    instance_->fillInPluginDescription(description_);
    instance_->setNonRealtime(true);
    instance_->setPlayConfigDetails(2, 2, sampleRate_, blockSize_);
    instance_->prepareToPlay(sampleRate_, blockSize_);
}

bool Vst3Plugin::SetParameterNormalized(const std::string& name, double value)
{
    const juce::String parameterName(name);
    for (auto* parameter : instance_->getParameters())
    {
        if (parameter != nullptr && parameter->getName(256).equalsIgnoreCase(parameterName))
        {
            const auto normalized = static_cast<float>(std::clamp(value, 0.0, 1.0));
            parameter->beginChangeGesture();
            parameter->setValueNotifyingHost(normalized);
            parameter->endChangeGesture();
            return true;
        }
    }
    return false;
}

bool Vst3Plugin::SetParameterText(const std::string& name, const std::string& text)
{
    const juce::String parameterName(name);
    for (auto* parameter : instance_->getParameters())
    {
        if (parameter != nullptr && parameter->getName(256).equalsIgnoreCase(parameterName))
        {
            const auto value = std::clamp(parameter->getValueForText(text), 0.0F, 1.0F);
            parameter->beginChangeGesture();
            parameter->setValueNotifyingHost(value);
            parameter->endChangeGesture();
            return true;
        }
    }
    return false;
}

std::vector<PluginParameterDescription> Vst3Plugin::Parameters() const
{
    std::vector<PluginParameterDescription> result;
    for (auto* parameter : instance_->getParameters())
    {
        if (parameter != nullptr)
        {
            const auto value = parameter->getValue();
            result.push_back({
                parameter->getName(256).toStdString(),
                parameter->getText(value, 256).toStdString(),
                value
            });
        }
    }
    return result;
}

void Vst3Plugin::Reprepare()
{
    instance_->releaseResources();
    instance_->setPlayConfigDetails(2, 2, sampleRate_, blockSize_);
    instance_->prepareToPlay(sampleRate_, blockSize_);
    instance_->reset();
}

std::size_t Vst3Plugin::LatencySamples() const noexcept
{
    return static_cast<std::size_t>(std::max(instance_->getLatencySamples(), 0));
}

std::string Vst3Plugin::Identity() const
{
    return description_.manufacturerName.toStdString() + "/" +
        description_.name.toStdString() + "/" +
        description_.version.toStdString() + "/" +
        description_.createIdentifierString().toStdString();
}

bool Vst3Plugin::LoadState(const std::filesystem::path& path)
{
    const juce::File file(path.string());
    if (!file.existsAsFile())
    {
        return false;
    }

    juce::MemoryBlock state;
    if (!file.loadFileAsData(state))
    {
        return false;
    }
    instance_->setStateInformation(state.getData(), static_cast<int>(state.getSize()));
    instance_->reset();
    return true;
}

bool Vst3Plugin::SaveState(const std::filesystem::path& path) const
{
    juce::MemoryBlock state;
    instance_->getStateInformation(state);
    const juce::File file(path.string());
    file.getParentDirectory().createDirectory();
    return file.replaceWithData(state.getData(), state.getSize());
}

void Vst3Plugin::Reset() noexcept
{
    instance_->reset();
}

std::vector<double> Vst3Plugin::Process(std::span<const double> input)
{
    return ProcessStereo(input, input).left;
}

StereoSignal Vst3Plugin::ProcessStereo(
    std::span<const double> leftInput,
    std::span<const double> rightInput)
{
    if (leftInput.size() != rightInput.size())
    {
        throw std::invalid_argument("Stereo input channel sizes differ.");
    }
    StereoSignal output{ std::vector<double>(leftInput.size()), std::vector<double>(leftInput.size()) };
    std::size_t position = 0;
    while (position < leftInput.size())
    {
        const auto frameCount = static_cast<int>(std::min<std::size_t>(
            static_cast<std::size_t>(blockSize_),
            leftInput.size() - position));
        buffer_.clear();
        midi_.clear();
        for (auto index = 0; index < frameCount; ++index)
        {
            buffer_.setSample(0, index, static_cast<float>(leftInput[position + index]));
            buffer_.setSample(1, index, static_cast<float>(rightInput[position + index]));
        }
        instance_->processBlock(buffer_, midi_);
        for (auto index = 0; index < frameCount; ++index)
        {
            output.left[position + index] = buffer_.getSample(0, index);
            output.right[position + index] = buffer_.getSample(1, index);
        }
        position += static_cast<std::size_t>(frameCount);
    }
    return output;
}

Vst3Host::Vst3Host()
{
    formats_.addDefaultFormats();
}

std::unique_ptr<Vst3Plugin> Vst3Host::Open(
    const std::filesystem::path& bundle,
    const std::string& query,
    double sampleRate,
    int blockSize,
    std::string& error)
{
    if (!std::filesystem::exists(bundle))
    {
        error = "VST3 bundle does not exist";
        return nullptr;
    }

    auto* format = formats_.getFormat(0);
    for (auto index = 0; index < formats_.getNumFormats(); ++index)
    {
        auto* candidate = formats_.getFormat(index);
        if (candidate != nullptr && candidate->getName().containsIgnoreCase("VST3"))
        {
            format = candidate;
            break;
        }
    }
    if (format == nullptr)
    {
        error = "VST3 format is unavailable";
        return nullptr;
    }

    juce::OwnedArray<juce::PluginDescription> descriptions;
    format->findAllTypesForFile(descriptions, bundle.string());
    const juce::PluginDescription* selected = nullptr;
    for (const auto* description : descriptions)
    {
        if (description != nullptr &&
            (query.empty() ||
                description->name.containsIgnoreCase(query) ||
                description->descriptiveName.containsIgnoreCase(query)))
        {
            selected = description;
            break;
        }
    }
    if (selected == nullptr)
    {
        error = "No matching plugin type was found in the VST3 bundle";
        return nullptr;
    }

    juce::String creationError;
    auto instance = formats_.createPluginInstance(
        *selected,
        sampleRate,
        blockSize,
        creationError);
    if (instance == nullptr)
    {
        error = creationError.toStdString();
        return nullptr;
    }
    return std::make_unique<Vst3Plugin>(std::move(instance), sampleRate, blockSize);
}

} // namespace emulation_lab
