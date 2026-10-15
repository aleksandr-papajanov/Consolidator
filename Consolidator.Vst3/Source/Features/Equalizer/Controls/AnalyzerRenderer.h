#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include <JuceHeader.h>

#include "Features/Equalizer/Presentation/AnalyzerCurveCalculator.h"

namespace consolidator::vst3::features::equalizer::controls
{

struct AnalyzerRenderState
{
    juce::Rectangle<float> plot;
    std::span<const features::equalizer::presentation::AnalyzerFilter> filters;
    juce::Colour groupColor;
    int selected;
    bool ready;
    bool groupScope;
    bool bankActive;
    bool hasSource;
    bool hasGroup;
    bool showAllBanks;
    bool showReference;
};

class AnalyzerRenderer final
{
public:
    void SetPlot(juce::Rectangle<float> plot);
    void UpdateCurves(const features::equalizer::presentation::AnalyzerCurves& curves,
        const std::optional<features::equalizer::presentation::AnalyzerCurves::Curve>& allBanks,
        std::uint64_t revision);
    void UpdateSpectrum(std::span<const double> source, std::span<const double> group);
    void UpdateReference(std::span<const double> reference);
    void Draw(juce::Graphics& graphics, const AnalyzerRenderState& state) const;
    [[nodiscard]] static juce::Point<float> HandlePosition(const features::equalizer::presentation::AnalyzerFilter& filter,
        const juce::Rectangle<float>& plot);

private:
    void BuildPath(juce::Path& path, std::span<const double> values) const;
    juce::Rectangle<float> plot_;
    std::optional<std::uint64_t> curvesRevision_;
    juce::Path gridLines_;
    juce::Path plotClip_;
    juce::Path gainHandle_;
    std::vector<juce::Path> filterPaths_;
    juce::Path combinedPath_;
    juce::Path allBanksPath_;
    juce::Path sourcePath_;
    juce::Path groupPath_;
    juce::Path referencePath_;
};

} // namespace consolidator::vst3::features::equalizer::controls
