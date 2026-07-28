#pragma once

#include "Audio/AnalyzerInputFrame.h"
#include "Settings/AnalysisOptions.h"
#include "Settings/AudioOptions.h"
#include "Helpers/NumericHelper.h"

#include <array>
#include <algorithm>
#include <cmath>

class AnalyzerFrameBuffer {
public:
    void Write(const consolidator::audio::AnalyzerInputFrame& frame) {
        currentLeft[writeIndex] = frame.current.left;
        currentRight[writeIndex] = frame.current.right;
        referenceLeft[writeIndex] = frame.reference.left;
        referenceRight[writeIndex] = frame.reference.right;
        maximumCurrentMagnitude = std::max({
            maximumCurrentMagnitude,
            std::abs(frame.current.left),
            std::abs(frame.current.right)
        });
        maximumReferenceMagnitude = std::max({
            maximumReferenceMagnitude,
            std::abs(frame.reference.left),
            std::abs(frame.reference.right)
        });
    }

    bool Advance(int fftSize = static_cast<int>(consolidator::settings::AnalysisOptions::DefaultFftSize)) {
        ++writeIndex;
        return writeIndex >= fftSize;
    }

    void Reset() {
        writeIndex = 0;
        maximumCurrentMagnitude = 0.0;
        maximumReferenceMagnitude = 0.0;
    }

    bool IsSilent() const noexcept {
        const auto threshold = std::pow(
            10.0,
            consolidator::settings::AudioOptions::SilenceThresholdDb / 20.0);
        return maximumCurrentMagnitude < threshold &&
            maximumReferenceMagnitude < threshold;
    }

    bool IsCurrentSilent() const noexcept {
        return maximumCurrentMagnitude < SilenceThreshold();
    }

    bool IsReferenceSilent() const noexcept {
        return maximumReferenceMagnitude < SilenceThreshold();
    }

    int WriteIndex() const {
        return writeIndex;
    }

    const std::array<double, consolidator::settings::AnalysisOptions::MaximumFftSize>& CurrentLeft() const {
        return currentLeft;
    }

    const std::array<double, consolidator::settings::AnalysisOptions::MaximumFftSize>& CurrentRight() const {
        return currentRight;
    }

    const std::array<double, consolidator::settings::AnalysisOptions::MaximumFftSize>& ReferenceLeft() const {
        return referenceLeft;
    }

    const std::array<double, consolidator::settings::AnalysisOptions::MaximumFftSize>& ReferenceRight() const {
        return referenceRight;
    }

    double ReferenceLevelDb() const noexcept {
        if (writeIndex <= 0) return -120.0;
        double energy = 0.0;
        for (int index = 0; index < writeIndex; ++index) {
            const auto offset = static_cast<std::size_t>(index);
            energy += referenceLeft[offset] * referenceLeft[offset] +
                referenceRight[offset] * referenceRight[offset];
        }
        const auto meanSquare = energy / (2.0 * static_cast<double>(writeIndex));
        return consolidator::helpers::NumericHelper::MagnitudeToDecibels(
            std::sqrt(std::max(meanSquare, 1.0e-12)));
    }

private:
    std::array<double, consolidator::settings::AnalysisOptions::MaximumFftSize> currentLeft{};
    std::array<double, consolidator::settings::AnalysisOptions::MaximumFftSize> currentRight{};
    std::array<double, consolidator::settings::AnalysisOptions::MaximumFftSize> referenceLeft{};
    std::array<double, consolidator::settings::AnalysisOptions::MaximumFftSize> referenceRight{};
    int writeIndex = 0;
    static double SilenceThreshold() noexcept {
        return std::pow(
            10.0,
            consolidator::settings::AudioOptions::SilenceThresholdDb / 20.0);
    }

    double maximumCurrentMagnitude = 0.0;
    double maximumReferenceMagnitude = 0.0;
};
