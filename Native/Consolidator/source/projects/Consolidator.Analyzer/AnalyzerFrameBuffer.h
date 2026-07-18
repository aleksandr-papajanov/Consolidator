#pragma once

#include "Audio/AnalyzerInputFrame.h"
#include "Settings/GlobalSettings.h"

#include <array>

class AnalyzerFrameBuffer {
public:
    void Write(const consolidator::audio::AnalyzerInputFrame& frame) {
        currentLeft[writeIndex] = frame.current.left;
        currentRight[writeIndex] = frame.current.right;
        referenceLeft[writeIndex] = frame.reference.left;
        referenceRight[writeIndex] = frame.reference.right;
    }

    bool Advance(int fftSize = static_cast<int>(consolidator::settings::GlobalSettings::DefaultFftSize)) {
        ++writeIndex;
        return writeIndex >= fftSize;
    }

    void Reset() {
        writeIndex = 0;
    }

    int WriteIndex() const {
        return writeIndex;
    }

    const std::array<double, consolidator::settings::GlobalSettings::MaximumFftSize>& CurrentLeft() const {
        return currentLeft;
    }

    const std::array<double, consolidator::settings::GlobalSettings::MaximumFftSize>& CurrentRight() const {
        return currentRight;
    }

    const std::array<double, consolidator::settings::GlobalSettings::MaximumFftSize>& ReferenceLeft() const {
        return referenceLeft;
    }

    const std::array<double, consolidator::settings::GlobalSettings::MaximumFftSize>& ReferenceRight() const {
        return referenceRight;
    }

private:
    std::array<double, consolidator::settings::GlobalSettings::MaximumFftSize> currentLeft{};
    std::array<double, consolidator::settings::GlobalSettings::MaximumFftSize> currentRight{};
    std::array<double, consolidator::settings::GlobalSettings::MaximumFftSize> referenceLeft{};
    std::array<double, consolidator::settings::GlobalSettings::MaximumFftSize> referenceRight{};
    int writeIndex = 0;
};
