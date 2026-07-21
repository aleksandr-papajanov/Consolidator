#pragma once

#include "IFeatureExtractor.h"
#include "FrequencyBands.h"
#include "Helpers/NumericHelper.h"
#include "Settings/AnalysisOptions.h"
#include "Settings/SpectrumOptions.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numeric>

namespace analysis_detail {

inline double StereoMagnitude(double left, double right) {
    return std::sqrt((left * left + right * right) * 0.5);
}

inline double ToDecibels(double magnitude) {
    return consolidator::helpers::NumericHelper::MagnitudeToDecibels(magnitude);
}

} // namespace analysis_detail

class RmsFeature final : public IFeatureExtractor {
public:
    std::string_view Id() const override { return "rms_db"; }

    double Extract(const SignalAnalysisWindow& window) override {
        if (window.left.empty()) return -120.0;
        double sum = 0.0;
        for (std::size_t index = 0; index < window.left.size(); ++index) {
            sum += (window.left[index] * window.left[index] +
                window.right[index] * window.right[index]) * 0.5;
        }
        return analysis_detail::ToDecibels(std::sqrt(sum / window.left.size()));
    }
};

class PeakFeature final : public IFeatureExtractor {
public:
    std::string_view Id() const override { return "peak_db"; }

    double Extract(const SignalAnalysisWindow& window) override {
        double peak = 0.0;
        for (std::size_t index = 0; index < window.left.size(); ++index) {
            peak = std::max(peak, std::max(std::abs(window.left[index]), std::abs(window.right[index])));
        }
        return analysis_detail::ToDecibels(peak);
    }
};

class CrestFeature final : public IFeatureExtractor {
public:
    std::string_view Id() const override { return "crest_db"; }

    double Extract(const SignalAnalysisWindow& window) override {
        if (window.left.empty()) return 0.0;
        double squareSum = 0.0;
        double peak = 0.0;
        for (std::size_t index = 0; index < window.left.size(); ++index) {
            const auto magnitude = analysis_detail::StereoMagnitude(window.left[index], window.right[index]);
            squareSum += magnitude * magnitude;
            peak = std::max(peak, magnitude);
        }
        const auto rms = std::sqrt(squareSum / window.left.size());
        return std::max(0.0, analysis_detail::ToDecibels(peak) - analysis_detail::ToDecibels(rms));
    }
};

class SpectralCentroidFeature final : public IFeatureExtractor {
public:
    std::string_view Id() const override { return "centroid_hz"; }

    double Extract(const SignalAnalysisWindow& window) override {
        double weighted = 0.0;
        double total = 0.0;
        for (std::size_t index = 1; index < window.spectrumMagnitudes.size(); ++index) {
            const auto magnitude = window.spectrumMagnitudes[index];
            weighted += window.FrequencyForBin(index) * magnitude;
            total += magnitude;
        }
        return total > 0.0 ? weighted / total : 0.0;
    }
};

class SpectralFlatnessFeature final : public IFeatureExtractor {
public:
    std::string_view Id() const override { return "flatness"; }

    double Extract(const SignalAnalysisWindow& window) override {
        if (window.spectrumMagnitudes.size() <= 1) return 0.0;
        double logarithmicSum = 0.0;
        double arithmeticSum = 0.0;
        const auto count = window.spectrumMagnitudes.size() - 1;
        for (std::size_t index = 1; index < window.spectrumMagnitudes.size(); ++index) {
            const auto magnitude = std::max(1.0e-20, window.spectrumMagnitudes[index]);
            logarithmicSum += std::log(magnitude);
            arithmeticSum += magnitude;
        }
        const auto geometricMean = std::exp(logarithmicSum / count);
        const auto arithmeticMean = arithmeticSum / count;
        return arithmeticMean > 0.0 ? std::clamp(geometricMean / arithmeticMean, 0.0, 1.0) : 0.0;
    }
};

class SpectralFluxFeature final : public IFeatureExtractor {
public:
    std::string_view Id() const override { return "flux"; }

    double Extract(const SignalAnalysisWindow& window) override {
        if (previousCount != window.spectrumMagnitudes.size()) {
            std::copy(window.spectrumMagnitudes.begin(), window.spectrumMagnitudes.end(), previous.begin());
            previousCount = window.spectrumMagnitudes.size();
            return 0.0;
        }
        double positiveChange = 0.0;
        double total = 0.0;
        for (std::size_t index = 1; index < previousCount; ++index) {
            const auto current = window.spectrumMagnitudes[index];
            positiveChange += std::max(0.0, current - previous[index]);
            total += current;
            previous[index] = current;
        }
        return total > 0.0 ? std::clamp(positiveChange / total, 0.0, 1.0) : 0.0;
    }

    void Reset() override { previousCount = 0; }

private:
    std::array<double, consolidator::settings::AnalysisOptions::MaximumFftSize / 2> previous{};
    std::size_t previousCount = 0;
};

class TransientEnergyFeature final : public IFeatureExtractor {
public:
    std::string_view Id() const override { return "transient_db"; }

    double Extract(const SignalAnalysisWindow& window) override {
        if (window.left.size() < 2) return -120.0;
        double sum = 0.0;
        for (std::size_t index = 1; index < window.left.size(); ++index) {
            const auto previous = analysis_detail::StereoMagnitude(window.left[index - 1], window.right[index - 1]);
            const auto current = analysis_detail::StereoMagnitude(window.left[index], window.right[index]);
            const auto rise = std::max(0.0, current - previous);
            sum += rise * rise;
        }
        return analysis_detail::ToDecibels(std::sqrt(sum / (window.left.size() - 1)));
    }
};

class BandFeatureBase {
public:
    explicit BandFeatureBase(FrequencyBand band) : band(band) {}

protected:
    bool Includes(const SignalAnalysisWindow& window, std::size_t index) const {
        const auto frequency = window.FrequencyForBin(index);
        return frequency >= band.minimumHz && frequency < band.maximumHz;
    }

    FrequencyBand band;
};

class BandRmsFeature final : public IFeatureExtractor, private BandFeatureBase {
public:
    explicit BandRmsFeature(FrequencyBand band) : BandFeatureBase(band) {}
    std::string_view Id() const override { return "rms_db"; }

    double Extract(const SignalAnalysisWindow& window) override {
        double sum = 0.0;
        std::size_t count = 0;
        for (std::size_t index = 1; index < window.spectrumMagnitudes.size(); ++index) {
            if (!Includes(window, index)) continue;
            const auto value = window.spectrumMagnitudes[index];
            sum += value * value;
            ++count;
        }
        return count ? analysis_detail::ToDecibels(std::sqrt(sum / count)) : -120.0;
    }
};

class BandPeakFeature final : public IFeatureExtractor, private BandFeatureBase {
public:
    explicit BandPeakFeature(FrequencyBand band) : BandFeatureBase(band) {}
    std::string_view Id() const override { return "peak_db"; }

    double Extract(const SignalAnalysisWindow& window) override {
        double peak = 0.0;
        for (std::size_t index = 1; index < window.spectrumMagnitudes.size(); ++index) {
            if (Includes(window, index)) peak = std::max(peak, window.spectrumMagnitudes[index]);
        }
        return analysis_detail::ToDecibels(peak);
    }
};

class BandCrestFeature final : public IFeatureExtractor, private BandFeatureBase {
public:
    explicit BandCrestFeature(FrequencyBand band) : BandFeatureBase(band) {}
    std::string_view Id() const override { return "crest_db"; }

    double Extract(const SignalAnalysisWindow& window) override {
        double sum = 0.0;
        double peak = 0.0;
        std::size_t count = 0;
        for (std::size_t index = 1; index < window.spectrumMagnitudes.size(); ++index) {
            if (!Includes(window, index)) continue;
            const auto value = window.spectrumMagnitudes[index];
            sum += value * value;
            peak = std::max(peak, value);
            ++count;
        }
        if (!count) return 0.0;
        return std::max(0.0, analysis_detail::ToDecibels(peak) -
            analysis_detail::ToDecibels(std::sqrt(sum / count)));
    }
};

class BandCentroidFeature final : public IFeatureExtractor, private BandFeatureBase {
public:
    explicit BandCentroidFeature(FrequencyBand band) : BandFeatureBase(band) {}
    std::string_view Id() const override { return "centroid_hz"; }

    double Extract(const SignalAnalysisWindow& window) override {
        double weighted = 0.0;
        double total = 0.0;
        for (std::size_t index = 1; index < window.spectrumMagnitudes.size(); ++index) {
            if (!Includes(window, index)) continue;
            const auto magnitude = window.spectrumMagnitudes[index];
            weighted += window.FrequencyForBin(index) * magnitude;
            total += magnitude;
        }
        return total > 0.0 ? weighted / total : (band.minimumHz + band.maximumHz) * 0.5;
    }
};

class BandFlatnessFeature final : public IFeatureExtractor, private BandFeatureBase {
public:
    explicit BandFlatnessFeature(FrequencyBand band) : BandFeatureBase(band) {}
    std::string_view Id() const override { return "flatness"; }

    double Extract(const SignalAnalysisWindow& window) override {
        double logarithmicSum = 0.0;
        double arithmeticSum = 0.0;
        std::size_t count = 0;
        for (std::size_t index = 1; index < window.spectrumMagnitudes.size(); ++index) {
            if (!Includes(window, index)) continue;
            const auto magnitude = std::max(1.0e-20, window.spectrumMagnitudes[index]);
            logarithmicSum += std::log(magnitude);
            arithmeticSum += magnitude;
            ++count;
        }
        if (!count || arithmeticSum <= 0.0) return 0.0;
        return std::clamp(std::exp(logarithmicSum / count) /
            (arithmeticSum / count), 0.0, 1.0);
    }
};

class BandFluxFeature final : public IFeatureExtractor, private BandFeatureBase {
public:
    explicit BandFluxFeature(FrequencyBand band) : BandFeatureBase(band) {}
    std::string_view Id() const override { return "flux"; }

    double Extract(const SignalAnalysisWindow& window) override {
        if (previousCount != window.spectrumMagnitudes.size()) {
            std::copy(window.spectrumMagnitudes.begin(), window.spectrumMagnitudes.end(), previous.begin());
            previousCount = window.spectrumMagnitudes.size();
            return 0.0;
        }
        double change = 0.0;
        double total = 0.0;
        for (std::size_t index = 1; index < previousCount; ++index) {
            const auto current = window.spectrumMagnitudes[index];
            if (Includes(window, index)) {
                change += std::max(0.0, current - previous[index]);
                total += current;
            }
            previous[index] = current;
        }
        return total > 0.0 ? std::clamp(change / total, 0.0, 1.0) : 0.0;
    }

    void Reset() override { previousCount = 0; }

private:
    std::array<double, consolidator::settings::AnalysisOptions::MaximumFftSize / 2> previous{};
    std::size_t previousCount = 0;
};

class BandTransientFeature final : public IFeatureExtractor, private BandFeatureBase {
public:
    explicit BandTransientFeature(FrequencyBand band) : BandFeatureBase(band) {}
    std::string_view Id() const override { return "transient_db"; }

    double Extract(const SignalAnalysisWindow& window) override {
        if (previousCount != window.spectrumMagnitudes.size()) {
            std::copy(window.spectrumMagnitudes.begin(), window.spectrumMagnitudes.end(), previous.begin());
            previousCount = window.spectrumMagnitudes.size();
            return -120.0;
        }
        double squareSum = 0.0;
        std::size_t count = 0;
        for (std::size_t index = 1; index < previousCount; ++index) {
            const auto current = window.spectrumMagnitudes[index];
            if (Includes(window, index)) {
                const auto rise = std::max(0.0, current - previous[index]);
                squareSum += rise * rise;
                ++count;
            }
            previous[index] = current;
        }
        return count ? analysis_detail::ToDecibels(std::sqrt(squareSum / count)) : -120.0;
    }

    void Reset() override { previousCount = 0; }

private:
    std::array<double, consolidator::settings::AnalysisOptions::MaximumFftSize / 2> previous{};
    std::size_t previousCount = 0;
};

class SpectralSimilarityFeature final {
public:
    static double Compare(
        const SignalAnalysisWindow& current,
        const SignalAnalysisWindow& reference
    ) {
        return CompareRange(
            current,
            reference,
            consolidator::settings::SpectrumOptions::MinimumFrequencyHz,
            consolidator::settings::SpectrumOptions::MaximumFrequencyHz);
    }

    static double CompareBand(
        const SignalAnalysisWindow& current,
        const SignalAnalysisWindow& reference,
        FrequencyBand band
    ) {
        return CompareRange(current, reference, band.minimumHz, band.maximumHz);
    }

private:
    static double CompareRange(
        const SignalAnalysisWindow& current,
        const SignalAnalysisWindow& reference,
        double minimumHz,
        double maximumHz
    ) {
        const auto count = std::min(
            current.spectrumMagnitudes.size(),
            reference.spectrumMagnitudes.size());
        double squaredDifference = 0.0;
        std::size_t comparedCount = 0;
        for (std::size_t index = 1; index < count; ++index) {
            const auto frequency = current.FrequencyForBin(index);
            if (frequency < minimumHz || frequency >= maximumHz) continue;
            const auto currentDb = std::clamp(
                analysis_detail::ToDecibels(current.spectrumMagnitudes[index]),
                consolidator::settings::SpectrumOptions::MinimumSpectrumDb,
                consolidator::settings::SpectrumOptions::MaximumSpectrumDb);
            const auto referenceDb = std::clamp(
                analysis_detail::ToDecibels(reference.spectrumMagnitudes[index]),
                consolidator::settings::SpectrumOptions::MinimumSpectrumDb,
                consolidator::settings::SpectrumOptions::MaximumSpectrumDb);
            const auto difference = referenceDb - currentDb;
            squaredDifference += difference * difference;
            ++comparedCount;
        }
        if (comparedCount == 0) return 1.0;
        const auto rmsDifference = std::sqrt(
            squaredDifference / static_cast<double>(comparedCount));
        return std::exp(
            -rmsDifference / consolidator::settings::AnalysisOptions::SpectralSimilarityScaleDb);
    }
};
