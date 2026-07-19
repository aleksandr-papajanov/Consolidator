#pragma once

#include "Audio/AnalyzerInputFrame.h"
#include "Settings/AnalysisOptions.h"

#include <array>

class AnalyzerFrameBuffer {
public:
    void Write(const consolidator::audio::AnalyzerInputFrame& frame) {
        currentLeft[writeIndex] = frame.current.left;
        currentRight[writeIndex] = frame.current.right;
        referenceLeft[writeIndex] = frame.reference.left;
        referenceRight[writeIndex] = frame.reference.right;
    }

    bool Advance(int fftSize = static_cast<int>(consolidator::settings::AnalysisOptions::DefaultFftSize)) {
        ++writeIndex;
        return writeIndex >= fftSize;
    }

    void Reset() {
        writeIndex = 0;
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

private:
    std::array<double, consolidator::settings::AnalysisOptions::MaximumFftSize> currentLeft{};
    std::array<double, consolidator::settings::AnalysisOptions::MaximumFftSize> currentRight{};
    std::array<double, consolidator::settings::AnalysisOptions::MaximumFftSize> referenceLeft{};
    std::array<double, consolidator::settings::AnalysisOptions::MaximumFftSize> referenceRight{};
    int writeIndex = 0;
};
