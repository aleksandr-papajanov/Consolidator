#include "TransientEnvelopeAnalysis.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <vector>
#include <JuceHeader.h>
#include <nlohmann/json.hpp>
namespace
{
struct Audio { double sampleRate{}; std::vector<double> energy; };
Audio ReadEnergy(const std::filesystem::path& path)
{
    juce::AudioFormatManager formats; formats.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(
        formats.createReaderFor(juce::File(path.string())));
    if (reader == nullptr || reader->numChannels != 2)
        throw std::runtime_error("Transient analysis requires stereo WAV files.");
    juce::AudioBuffer<float> buffer(2, static_cast<int>(reader->lengthInSamples));
    if (!reader->read(&buffer, 0, buffer.getNumSamples(), 0, true, true))
        throw std::runtime_error("Could not read transient-analysis WAV.");
    Audio result{ reader->sampleRate, std::vector<double>(reader->lengthInSamples) };
    for (auto index = 0; index < buffer.getNumSamples(); ++index)
    {
        const auto left = static_cast<double>(buffer.getSample(0, index));
        const auto right = static_cast<double>(buffer.getSample(1, index));
        result.energy[static_cast<std::size_t>(index)] =
            0.5 * (left * left + right * right);
    }
    return result;
}
double GainDb(const Audio& output, const Audio& dry,
    std::size_t start, std::size_t count)
{
    auto outputEnergy = 0.0; auto dryEnergy = 0.0;
    for (auto index = start; index < start + count; ++index)
    { outputEnergy += output.energy[index]; dryEnergy += dry.energy[index]; }
    return 10.0 * std::log10(std::max(outputEnergy, 1.0e-24) /
        std::max(dryEnergy, 1.0e-24));
}

double EnvelopeCorrelation(const Audio& dry, std::size_t reference,
    std::size_t candidate, std::size_t count)
{
    auto product = 0.0;
    auto referenceEnergy = 0.0;
    auto candidateEnergy = 0.0;
    for (auto index = std::size_t{}; index < count; ++index)
    {
        const auto referenceValue = std::sqrt(dry.energy[reference + index]);
        const auto candidateValue = std::sqrt(dry.energy[candidate + index]);
        product += referenceValue * candidateValue;
        referenceEnergy += referenceValue * referenceValue;
        candidateEnergy += candidateValue * candidateValue;
    }
    return product / std::sqrt(std::max(
        referenceEnergy * candidateEnergy, 1.0e-24));
}
}
namespace emulation_lab
{
void AnalyzeTransientEnvelopes(const std::filesystem::path& dryPath,
    const std::filesystem::path& referencePath,
    const std::filesystem::path& candidatePath,
    const std::filesystem::path& outputPath)
{
    const auto dry = ReadEnergy(dryPath); const auto reference = ReadEnergy(referencePath);
    const auto candidate = ReadEnergy(candidatePath);
    if (reference.sampleRate != dry.sampleRate || candidate.sampleRate != dry.sampleRate ||
        reference.energy.size() != dry.energy.size() || candidate.energy.size() != dry.energy.size())
        throw std::runtime_error("Transient-analysis WAV formats do not match.");
    const auto refractory = static_cast<std::size_t>(0.1 * dry.sampleRate);
    const auto lookback = static_cast<std::size_t>(0.005 * dry.sampleRate);
    const auto preRoll = static_cast<std::size_t>(0.020 * dry.sampleRate);
    const auto tail = static_cast<std::size_t>(0.150 * dry.sampleRate);
    std::vector<std::size_t> onsets;
    for (auto index = std::max(lookback, preRoll);
        index + tail < dry.energy.size(); ++index)
    {
        if (dry.energy[index] > 0.04 && dry.energy[index] > 4.0 * dry.energy[index - lookback])
        { onsets.push_back(index); index += refractory; }
    }
    std::ranges::sort(onsets, [&dry](auto left, auto right)
    { return dry.energy[left] > dry.energy[right]; });
    if (onsets.size() > 12) onsets.resize(12);
    const auto alignmentRadius = static_cast<std::size_t>(0.005 * dry.sampleRate);
    const auto alignmentLength = static_cast<std::size_t>(0.010 * dry.sampleRate);
    const auto alignmentReference = onsets.empty() ? preRoll : onsets.front();
    nlohmann::json alignments = nlohmann::json::array();
    for (auto& onset : onsets)
    {
        const auto original = onset;
        auto best = onset;
        auto bestCorrelation = -1.0;
        for (auto offset = -static_cast<std::ptrdiff_t>(alignmentRadius);
            offset <= static_cast<std::ptrdiff_t>(alignmentRadius); ++offset)
        {
            const auto shifted = static_cast<std::ptrdiff_t>(onset) + offset;
            if (shifted < static_cast<std::ptrdiff_t>(preRoll) ||
                static_cast<std::size_t>(shifted) + alignmentLength >= dry.energy.size())
            {
                continue;
            }
            const auto correlation = EnvelopeCorrelation(dry, alignmentReference,
                static_cast<std::size_t>(shifted), alignmentLength);
            if (correlation > bestCorrelation)
            {
                bestCorrelation = correlation;
                best = static_cast<std::size_t>(shifted);
            }
        }
        onset = best;
        alignments.push_back({
            { "originalSample", original },
            { "alignedSample", best },
            { "offsetMs", 1000.0 * static_cast<double>(
                static_cast<std::ptrdiff_t>(best) -
                static_cast<std::ptrdiff_t>(original)) / dry.sampleRate },
            { "correlation", bestCorrelation }
        });
    }
    const auto bin = std::max<std::size_t>(static_cast<std::size_t>(0.001 * dry.sampleRate), 1);
    nlohmann::json points = nlohmann::json::array();
    for (auto offsetMs = -20; offsetMs < 150; ++offsetMs)
    {
        auto referenceGain = 0.0; auto candidateGain = 0.0;
        for (const auto onset : onsets)
        {
            const auto start = static_cast<std::size_t>(static_cast<std::ptrdiff_t>(onset) +
                static_cast<std::ptrdiff_t>(offsetMs * static_cast<int>(bin)));
            referenceGain += GainDb(reference, dry, start, bin);
            candidateGain += GainDb(candidate, dry, start, bin);
        }
        const auto count = std::max<std::size_t>(onsets.size(), 1);
        points.push_back({ { "offsetMs", offsetMs },
            { "referenceGainDb", referenceGain / count },
            { "candidateGainDb", candidateGain / count },
            { "errorDb", (candidateGain - referenceGain) / count } });
    }
    nlohmann::json report{ { "schemaVersion", 1 }, { "sampleRate", dry.sampleRate },
        { "onsetCount", onsets.size() }, { "alignments", std::move(alignments) },
        { "points", std::move(points) } };
    if (!outputPath.parent_path().empty()) std::filesystem::create_directories(outputPath.parent_path());
    std::ofstream output(outputPath); output << report.dump(2) << '\n';
}
}
