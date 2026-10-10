#include "AudioFileRender.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <stdexcept>
#include <vector>

#include "Host/Vst3Host.h"

namespace emulation_lab
{

void RenderAudioFile(
    const ExperimentManifest& manifest,
    const std::string& side,
    const std::string& conditionId,
    const std::filesystem::path& inputPath,
    const std::filesystem::path& outputPath,
    const std::vector<std::pair<std::string, double>>& overrides)
{
    if (side != "reference" && side != "candidate")
    {
        throw std::runtime_error("Render side must be reference or candidate.");
    }
    if (std::filesystem::absolute(inputPath).lexically_normal() ==
        std::filesystem::absolute(outputPath).lexically_normal())
    {
        throw std::runtime_error("Render output must differ from its input WAV.");
    }
    if (std::filesystem::exists(outputPath))
    {
        throw std::runtime_error("Render output already exists: " + outputPath.string());
    }
    const auto condition = std::find_if(
        manifest.conditions.begin(), manifest.conditions.end(),
        [&conditionId](const auto& value) { return value.identifier == conditionId; });
    if (condition == manifest.conditions.end())
    {
        throw std::runtime_error("Render condition was not found: " + conditionId);
    }

    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(
        formats.createReaderFor(juce::File(inputPath.string())));
    if (reader == nullptr || reader->numChannels != 2 || reader->sampleRate <= 0.0)
    {
        throw std::runtime_error("Input must be a readable stereo WAV file.");
    }

    const auto reference = side == "reference";
    const auto& pluginPath = reference ? manifest.referencePlugin : manifest.candidatePlugin;
    const auto& query = reference ? manifest.referenceQuery : manifest.candidateQuery;
    const auto& controls = reference ? condition->referenceControls : condition->candidateControls;
    const auto& state = reference ? condition->referenceState : condition->candidateState;
    const auto& baseState = reference ? manifest.referenceState : manifest.candidateState;
    const auto blockSize = static_cast<int>(manifest.blockSize);
    Vst3Host host;
    std::string error;
    auto plugin = host.Open(pluginPath, query, reader->sampleRate, blockSize, error);
    if (plugin == nullptr)
    {
        throw std::runtime_error(error);
    }
    const auto& effectiveState = state.empty() ? baseState : state;
    if (!effectiveState.empty() && !plugin->LoadState(effectiveState))
    {
        throw std::runtime_error("Could not load render plugin state: " + effectiveState);
    }
    for (const auto& control : controls)
    {
        const auto applied = control.usesNormalizedValue
            ? plugin->SetParameterNormalized(control.name, control.normalizedValue)
            : plugin->SetParameterText(control.name, control.text);
        if (!applied)
        {
            throw std::runtime_error("Could not apply render control: " + control.name);
        }
    }
    for (const auto& [name, value] : overrides)
    {
        if (!std::isfinite(value) || value < 0.0 || value > 1.0 ||
            !plugin->SetParameterNormalized(name, value))
        {
            throw std::runtime_error("Invalid render override: " + name);
        }
    }
    plugin->Reprepare();

    const auto parent = outputPath.parent_path();
    if (!parent.empty())
    {
        std::filesystem::create_directories(parent);
    }
    auto stream = std::make_unique<juce::FileOutputStream>(juce::File(outputPath.string()));
    if (stream->failedToOpen())
    {
        throw std::runtime_error("Could not open render output: " + outputPath.string());
    }
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::AudioFormatWriter> writer(wav.createWriterFor(
        stream.get(), reader->sampleRate, 2, 32, {}, 0));
    if (writer == nullptr)
    {
        throw std::runtime_error("Could not create WAV writer.");
    }
    stream.release();

    juce::AudioBuffer<float> input(2, blockSize);
    std::vector<double> left(static_cast<std::size_t>(blockSize));
    std::vector<double> right(static_cast<std::size_t>(blockSize));
    juce::AudioBuffer<float> output(2, blockSize);
    for (juce::int64 position = 0; position < reader->lengthInSamples; position += blockSize)
    {
        const auto count = static_cast<int>(std::min<juce::int64>(
            blockSize, reader->lengthInSamples - position));
        input.clear();
        if (!reader->read(&input, 0, count, position, true, true))
        {
            throw std::runtime_error("Could not read input WAV samples.");
        }
        for (auto index = 0; index < count; ++index)
        {
            left[static_cast<std::size_t>(index)] = input.getSample(0, index);
            right[static_cast<std::size_t>(index)] = input.getSample(1, index);
        }
        const auto rendered = plugin->ProcessStereo(
            std::span<const double>(left.data(), static_cast<std::size_t>(count)),
            std::span<const double>(right.data(), static_cast<std::size_t>(count)));
        for (auto index = 0; index < count; ++index)
        {
            output.setSample(0, index, static_cast<float>(rendered.left[static_cast<std::size_t>(index)]));
            output.setSample(1, index, static_cast<float>(rendered.right[static_cast<std::size_t>(index)]));
        }
        if (!writer->writeFromAudioSampleBuffer(output, 0, count))
        {
            throw std::runtime_error("Could not write rendered WAV samples.");
        }
    }
    writer->flush();
}

} // namespace emulation_lab
