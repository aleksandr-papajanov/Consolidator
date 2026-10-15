#pragma once

#include <cstddef>
#include <vector>

#include "Features/Equalizer/Clients/AnalyzerData.h"

namespace consolidator::vst3::features::equalizer::presentation
{

// Display smoothing and peak-preserving projection; Managed supplies the FFT.
class SpectrumPresentation final
{
public:
    void Clear();
    void SetSampleRate(double sampleRate);
    void SetResolution(std::size_t pointCount);
    void Apply(clients::AnalyzerSpectrum spectrum);
    [[nodiscard]] const std::vector<double>& Source() const;
    [[nodiscard]] const std::vector<double>& Group() const;
    [[nodiscard]] const std::vector<double>& Reference() const;

private:
    struct Interval
    {
        double center;
        double left;
        double right;
    };

    void Resample(const std::vector<double>& bins, std::vector<double>& result) const;
    std::vector<double> sourceBins_;
    std::vector<double> groupBins_;
    std::vector<double> referenceBins_;
    std::vector<double> sourceSpectrum_;
    std::vector<double> groupSpectrum_;
    std::vector<double> referenceSpectrum_;
    std::vector<Interval> intervals_;
    double sampleRate_{};
    std::size_t fftSize_{};
};

} // namespace consolidator::vst3::features::equalizer::presentation
