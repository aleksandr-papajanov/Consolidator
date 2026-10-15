#pragma once

#include <array>
#include <cstdint>

#include <JuceHeader.h>

namespace consolidator::vst3::shared::ui
{

// Native presentation of Consolidator.Max/js/Shared/Theme/UiColors.js.
inline const auto kBackground = juce::Colour::fromFloatRGBA(0.0F, 0.0F, 0.0F, 1.0F);
inline const auto kText = juce::Colour::fromFloatRGBA(0.52F, 0.55F, 0.59F, 1.0F);
inline const auto kDisabled = juce::Colour::fromFloatRGBA(0.44F, 0.46F, 0.48F, 1.0F);
inline const auto kActiveText = juce::Colour::fromFloatRGBA(0.80F, 0.85F, 0.90F, 1.0F);
inline const auto kLines = juce::Colour::fromFloatRGBA(0.14F, 0.15F, 0.16F, 1.0F);
inline const auto kActive = juce::Colour::fromFloatRGBA(0.72F, 0.88F, 0.28F, 1.0F);
inline const auto kMute = juce::Colour::fromFloatRGBA(0.91F, 0.35F, 0.32F, 1.0F);
inline const auto kReset = juce::Colour::fromFloatRGBA(0.82F, 0.68F, 0.32F, 1.0F);
inline const auto kAnalyzerBackground = juce::Colour::fromFloatRGBA(0.025F, 0.027F, 0.024F, 1.0F);
inline const auto kAnalyzerSpectrum = juce::Colour::fromFloatRGBA(0.75F, 0.76F, 0.69F, 0.58F);
inline const auto kAnalyzerCombinedInactive = juce::Colour::fromFloatRGBA(0.35F, 0.37F, 0.32F, 0.55F);
inline const auto kAnalyzerHandle = juce::Colour::fromFloatRGBA(0.93F, 0.64F, 0.26F, 1.0F);
inline const auto kAnalyzerAllBanks = juce::Colour::fromFloatRGBA(0.96F, 0.86F, 0.42F, 1.0F);
inline const auto kAnalyzerSelectedHandle = juce::Colour::fromFloatRGBA(1.0F, 0.82F, 0.32F, 1.0F);
inline const std::array<juce::Colour, 7> kAnalyzerFilters
{
    juce::Colour::fromFloatRGBA(0.93F, 0.37F, 0.31F, 0.90F),
    juce::Colour::fromFloatRGBA(0.94F, 0.55F, 0.25F, 0.90F),
    juce::Colour::fromFloatRGBA(0.88F, 0.72F, 0.27F, 0.90F),
    juce::Colour::fromFloatRGBA(0.63F, 0.82F, 0.30F, 0.90F),
    juce::Colour::fromFloatRGBA(0.28F, 0.71F, 0.58F, 0.90F),
    juce::Colour::fromFloatRGBA(0.28F, 0.62F, 0.79F, 0.90F),
    juce::Colour::fromFloatRGBA(0.65F, 0.44F, 0.79F, 0.90F)
};
inline const auto kGainStaging = juce::Colour::fromFloatRGBA(0.30F, 0.66F, 0.72F, 1.0F);
inline const auto kSaturator = juce::Colour::fromFloatRGBA(0.90F, 0.48F, 0.27F, 1.0F);
inline const auto kShaper = juce::Colour::fromFloatRGBA(0.64F, 0.44F, 0.77F, 1.0F);
inline const auto kCompression = juce::Colour::fromFloatRGBA(0.52F, 0.70F, 0.36F, 1.0F);
inline const auto kEqualizer = juce::Colour::fromFloatRGBA(0.64F, 0.80F, 0.31F, 1.0F);
inline const auto kPolish = juce::Colour::fromFloatRGBA(0.78F, 0.40F, 0.58F, 1.0F);
inline const std::array<juce::Colour, 16> kGroups
{
    juce::Colour::fromFloatRGBA(0.48F, 0.79F, 0.36F, 1.0F),
    juce::Colour::fromFloatRGBA(0.28F, 0.72F, 0.51F, 1.0F),
    juce::Colour::fromFloatRGBA(0.25F, 0.68F, 0.65F, 1.0F),
    juce::Colour::fromFloatRGBA(0.30F, 0.61F, 0.75F, 1.0F),
    juce::Colour::fromFloatRGBA(0.39F, 0.53F, 0.78F, 1.0F),
    juce::Colour::fromFloatRGBA(0.51F, 0.47F, 0.78F, 1.0F),
    juce::Colour::fromFloatRGBA(0.64F, 0.43F, 0.73F, 1.0F),
    juce::Colour::fromFloatRGBA(0.74F, 0.42F, 0.63F, 1.0F),
    juce::Colour::fromFloatRGBA(0.83F, 0.43F, 0.49F, 1.0F),
    juce::Colour::fromFloatRGBA(0.88F, 0.49F, 0.34F, 1.0F),
    juce::Colour::fromFloatRGBA(0.91F, 0.60F, 0.29F, 1.0F),
    juce::Colour::fromFloatRGBA(0.88F, 0.70F, 0.27F, 1.0F),
    juce::Colour::fromFloatRGBA(0.72F, 0.74F, 0.28F, 1.0F),
    juce::Colour::fromFloatRGBA(0.38F, 0.75F, 0.59F, 1.0F),
    juce::Colour::fromFloatRGBA(0.66F, 0.51F, 0.69F, 1.0F),
    juce::Colour::fromFloatRGBA(0.76F, 0.86F, 0.31F, 1.0F)
};

inline juce::Colour GroupColor(std::uint32_t group) { return kGroups[group % kGroups.size()]; }

} // namespace consolidator::vst3::shared::ui
