#include "Features/Equalizer/Presentation/SpectrumPresentation.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

#include "Features/Equalizer/Presentation/AnalyzerScale.h"

namespace consolidator::vst3::features::equalizer::presentation
{

namespace
{
void Smooth(std::vector<double>& previous, std::vector<double> current)
{
    if (previous.size() == current.size())
    {
        for (std::size_t index = 0; index < current.size(); ++index)
        {
            const double coefficient = current[index] < previous[index] ? 0.5 : 0.16;
            current[index] = previous[index] + (current[index] - previous[index]) * coefficient;
        }
    }
    previous = std::move(current);
}
}

void SpectrumPresentation::Clear()
{
    sourceBins_.clear();
    groupBins_.clear();
    referenceBins_.clear();
    sourceSpectrum_.clear();
    groupSpectrum_.clear();
    referenceSpectrum_.clear();
    fftSize_ = 0;
}

void SpectrumPresentation::SetSampleRate(double sampleRate)
{
    if (sampleRate_ != sampleRate)
    {
        sampleRate_ = sampleRate;
        Clear();
    }
}

void SpectrumPresentation::Resample(const std::vector<double>& bins, std::vector<double>& result) const
{
    result.clear();
    if (bins.empty() || intervals_.empty())
    {
        return;
    }
    result.resize(intervals_.size());
    const double lastBin = static_cast<double>(bins.size() - 1);
    const double binScale = static_cast<double>(fftSize_) / sampleRate_;
    const auto interpolate = [&bins, lastBin](double bin)
    {
        bin = std::clamp(bin, 0.0, lastBin);
        const auto lower = static_cast<std::size_t>(std::floor(bin));
        const auto upper = std::min(lower + 1, bins.size() - 1);
        return bins[lower] + (bins[upper] - bins[lower]) * (bin - static_cast<double>(lower));
    };
    for (std::size_t point = 0; point < result.size(); ++point)
    {
        const auto& interval = intervals_[point];
        const double center = interval.center * binScale;
        if (center > lastBin)
        {
            result[point] = std::numeric_limits<double>::quiet_NaN();
            continue;
        }
        const double left = interval.left * binScale;
        const double right = std::min(lastBin, interval.right * binScale);
        // Coordinates run downwards: the minimum preserves the loudest bin in a plot column.
        double peak = std::min(interpolate(left), interpolate(right));
        for (auto bin = static_cast<std::size_t>(std::ceil(left));
            bin <= static_cast<std::size_t>(std::floor(right)); ++bin)
        {
            peak = std::min(peak, bins[bin]);
        }
        result[point] = peak;
    }
}

void SpectrumPresentation::SetResolution(std::size_t pointCount)
{
    if (pointCount < 2)
    {
        pointCount = 0;
    }
    if (intervals_.size() == pointCount)
    {
        return;
    }
    intervals_.resize(pointCount);
    for (std::size_t point = 0; point < pointCount; ++point)
    {
        const double denominator = static_cast<double>(pointCount - 1);
        intervals_[point] = {
            AnalyzerScale::Frequency(static_cast<double>(point) / denominator),
            AnalyzerScale::Frequency(std::max(0.0, static_cast<double>(point) - 0.5) / denominator),
            AnalyzerScale::Frequency(std::min(denominator, static_cast<double>(point) + 0.5) / denominator)
        };
    }
    Resample(sourceBins_, sourceSpectrum_);
    Resample(groupBins_, groupSpectrum_);
    Resample(referenceBins_, referenceSpectrum_);
}

void SpectrumPresentation::Apply(clients::AnalyzerSpectrum spectrum)
{
    fftSize_ = spectrum.fftSize;
    Smooth(sourceBins_, std::move(spectrum.source));
    Smooth(groupBins_, std::move(spectrum.group));
    Smooth(referenceBins_, std::move(spectrum.reference));
    Resample(sourceBins_, sourceSpectrum_);
    Resample(groupBins_, groupSpectrum_);
    Resample(referenceBins_, referenceSpectrum_);
}

const std::vector<double>& SpectrumPresentation::Source() const { return sourceSpectrum_; }
const std::vector<double>& SpectrumPresentation::Group() const { return groupSpectrum_; }
const std::vector<double>& SpectrumPresentation::Reference() const { return referenceSpectrum_; }

} // namespace consolidator::vst3::features::equalizer::presentation
