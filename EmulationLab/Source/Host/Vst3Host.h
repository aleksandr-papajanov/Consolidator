#pragma once

#include "Probe.h"

#include <JuceHeader.h>

#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace emulation_lab
{

struct PluginParameterDescription
{
    std::string name;
    std::string text;
    double normalizedValue{};
};

class Vst3Plugin final
{
public:
    Vst3Plugin(
        std::unique_ptr<juce::AudioPluginInstance> instance,
        double sampleRate,
        int blockSize);

    [[nodiscard]] bool LoadState(const std::filesystem::path& path);
    [[nodiscard]] bool SaveState(const std::filesystem::path& path) const;

    [[nodiscard]] bool SetParameterNormalized(
        const std::string& name,
        double value);
    [[nodiscard]] bool SetParameterText(
        const std::string& name,
        const std::string& text);

    [[nodiscard]] std::vector<PluginParameterDescription> Parameters() const;
    [[nodiscard]] bool Parameter(
        const std::string& name,
        PluginParameterDescription& description) const;

    void Reprepare();

    [[nodiscard]] std::size_t LatencySamples() const noexcept;
    [[nodiscard]] std::string Identity() const;

    void Reset() noexcept;

    [[nodiscard]] std::vector<double> Process(
        std::span<const double> input);
    [[nodiscard]] StereoSignal ProcessStereo(
        std::span<const double> left,
        std::span<const double> right);

private:
    std::unique_ptr<juce::AudioPluginInstance> instance_;
    double sampleRate_{};
    int blockSize_{};
    juce::PluginDescription description_;
    juce::AudioBuffer<float> buffer_;
    juce::MidiBuffer midi_;
};

class Vst3Host final
{
public:
    Vst3Host();

    [[nodiscard]] std::unique_ptr<Vst3Plugin> Open(
        const std::filesystem::path& bundle,
        const std::string& query,
        double sampleRate,
        int blockSize,
        std::string& error);

private:
    juce::AudioPluginFormatManager formats_;
};

} // namespace emulation_lab
