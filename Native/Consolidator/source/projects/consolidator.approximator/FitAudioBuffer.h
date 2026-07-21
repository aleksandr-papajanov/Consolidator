#pragma once

#include "Settings/FitOptions.h"

#include <algorithm>
#include <cstddef>
#include <vector>

class FitAudioBuffer final {
public:
    void Prepare(double nextSampleRate) {
        sampleRate = nextSampleRate;
        const auto size = static_cast<std::size_t>(
            std::max(1.0, sampleRate * consolidator::settings::FitOptions::CaptureSeconds));
        currentLeft.assign(size, 0.0);
        currentRight.assign(size, 0.0);
        referenceLeft.assign(size, 0.0);
        referenceRight.assign(size, 0.0);
    }

    void Write(std::size_t index, double currentL, double currentR, double referenceL, double referenceR) {
        currentLeft[index] = currentL;
        currentRight[index] = currentR;
        referenceLeft[index] = referenceL;
        referenceRight[index] = referenceR;
    }

    std::size_t Size() const noexcept { return currentLeft.size(); }
    double SampleRate() const noexcept { return sampleRate; }
    const std::vector<double>& CurrentLeft() const noexcept { return currentLeft; }
    const std::vector<double>& CurrentRight() const noexcept { return currentRight; }
    const std::vector<double>& ReferenceLeft() const noexcept { return referenceLeft; }
    const std::vector<double>& ReferenceRight() const noexcept { return referenceRight; }

private:
    double sampleRate = 48000.0;
    std::vector<double> currentLeft;
    std::vector<double> currentRight;
    std::vector<double> referenceLeft;
    std::vector<double> referenceRight;
};
