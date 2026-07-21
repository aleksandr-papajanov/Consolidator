#pragma once

#include "AnalyzerFrameBuffer.h"
#include "AnalyzerCurveBatch.h"
#include "AnalyzerSpectrumResult.h"
#include "DSP/Spectrum/FftEngine.h"
#include "Settings/AnalysisOptions.h"
#include "Settings/AudioOptions.h"
#include "Settings/SpectrumOptions.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <numbers>
#include <span>
#include <utility>

class AnalyzerSpectrumEngine {
public:
    void SetSampleRate(double sampleRate) {
        this->sampleRate = sampleRate;
    }

    AnalyzerSpectrumResult Analyze(
        const AnalyzerFrameBuffer& frame,
        AnalyzerCurveBatch& curves
    ) {
        const int fftSize = static_cast<int>(consolidator::settings::AnalysisOptions::DefaultFftSize);
        const int binsOut = static_cast<int>(consolidator::settings::AnalysisOptions::DefaultCurvePointCount);
        auto referenceSpectrum = StereoSpectrum(
            frame.ReferenceLeft(),
            frame.ReferenceRight(),
            frame.WriteIndex(),
            fftSize);

        auto currentSpectrum = StereoSpectrum(
            frame.CurrentLeft(),
            frame.CurrentRight(),
            frame.WriteIndex(),
            fftSize);
        const int previousPendingCount = curves.Prepare();

        for (int i = 0; i < binsOut; ++i) {
            curves.StoreBin(
                i,
                previousPendingCount,
                SampleSpectrumDb(currentSpectrum.decibels, i, binsOut, fftSize),
                SampleSpectrumDb(referenceSpectrum.decibels, i, binsOut, fftSize));
        }

        curves.FinalizeFrame();
        return { std::move(currentSpectrum), std::move(referenceSpectrum) };
    }

private:
    AnalyzerSignalSpectrum StereoSpectrum(
        const std::array<double, consolidator::settings::AnalysisOptions::MaximumFftSize>& left,
        const std::array<double, consolidator::settings::AnalysisOptions::MaximumFftSize>& right,
        int writeIndex,
        int fftSize
    ) {
        MakeWindowedCopy(left, writeIndex, fftSize, windowedSamples);
        Magnitudes(
            std::span{ windowedSamples.data(), static_cast<std::size_t>(fftSize) },
            std::span{ leftMagnitudes.data(), static_cast<std::size_t>(fftSize / 2) });
        std::copy_n(fftOutput.begin(), fftSize / 2, leftFft.begin());
        MakeWindowedCopy(right, writeIndex, fftSize, windowedSamples);
        Magnitudes(
            std::span{ windowedSamples.data(), static_cast<std::size_t>(fftSize) },
            std::span{ rightMagnitudes.data(), static_cast<std::size_t>(fftSize / 2) });
        AnalyzerSignalSpectrum result;
        result.pointCount = static_cast<std::size_t>(fftSize / 2);

        for (int i = 0; i < fftSize / 2; ++i) {
            result.magnitudes[i] = (leftMagnitudes[i] + rightMagnitudes[i]) * 0.5;
            result.decibels[i] = consolidator::helpers::NumericHelper::MagnitudeToDecibels(
                result.magnitudes[i]);
            result.leftPowers[i] = std::norm(leftFft[i]);
            result.rightPowers[i] = std::norm(fftOutput[i]);
            result.crossPowers[i] = std::real(leftFft[i] * std::conj(fftOutput[i]));
        }

        return result;
    }

    void MakeWindowedCopy(
        const std::array<double, consolidator::settings::AnalysisOptions::MaximumFftSize>& source,
        int writeIndex,
        int fftSize,
        std::array<double, consolidator::settings::AnalysisOptions::MaximumFftSize>& output
    ) const {
        const int start = writeIndex;

        for (int i = 0; i < fftSize; ++i) {
            const int index = (start + i) % fftSize;
            const double hann = consolidator::settings::AnalysisOptions::HannWindowCoefficient * (1.0 - std::cos(
                (2.0 * std::numbers::pi * i) / (fftSize - 1)));

            output[i] = source[index] * hann;
        }

    }

    void Magnitudes(std::span<const double> input, std::span<double> magnitudes) {
        engine.Forward(input, std::span{ fftOutput.data(), input.size() });
        const int fftSize = static_cast<int>(engine.Size());
        const double coherentGain = consolidator::settings::AnalysisOptions::HannWindowCoherentGain;
        const double amplitudeScale = static_cast<double>(fftSize) * coherentGain *
            consolidator::settings::AnalysisOptions::SingleSidedSpectrumScale;

        for (int i = 0; i < fftSize / 2; ++i) {
            const double re = fftOutput[static_cast<std::size_t>(i)].real();
            const double im = fftOutput[static_cast<std::size_t>(i)].imag();
            magnitudes[i] = std::sqrt(re * re + im * im) / amplitudeScale;
        }
    }

    double MapOutputBinToFftBin(int index, int binsOut, int fftSize) const {
        if (binsOut <= 1) {
            return 0.0;
        }

        const double normalized = static_cast<double>(index) / static_cast<double>(binsOut - 1);
        const int maxBin = (fftSize / 2) - 1;
        const double nyquist = sampleRate * 0.5;
        const double maximumFrequency = std::max(
            consolidator::settings::SpectrumOptions::MinimumFrequencyHz,
            std::min(consolidator::settings::SpectrumOptions::MaximumFrequencyHz, nyquist));
        const double frequencyHz = consolidator::settings::SpectrumOptions::MinimumFrequencyHz *
            std::pow(maximumFrequency /
                consolidator::settings::SpectrumOptions::MinimumFrequencyHz, normalized);
        const double mappedBin = frequencyHz * static_cast<double>(fftSize) / sampleRate;

        return std::clamp(mappedBin, 1.0, static_cast<double>(maxBin));
    }

    double SampleSpectrumDb(
        const std::array<double, AnalyzerSignalSpectrum::MaximumBinCount>& values,
        int index,
        int binsOut,
        int fftSize
    ) const {
        const auto mappedBin = MapOutputBinToFftBin(index, binsOut, fftSize);
        const auto lower = static_cast<std::size_t>(std::floor(mappedBin));
        const auto upper = std::min(lower + 1, static_cast<std::size_t>(fftSize / 2 - 1));
        const auto fraction = mappedBin - static_cast<double>(lower);
        return values[lower] + (values[upper] - values[lower]) * fraction;
    }

    double sampleRate = consolidator::settings::AudioOptions::DefaultSampleRateHz;
    consolidator::dsp::FftEngine engine;
    std::array<double, consolidator::settings::AnalysisOptions::MaximumFftSize> windowedSamples{};
    std::array<double, AnalyzerSignalSpectrum::MaximumBinCount> leftMagnitudes{};
    std::array<double, AnalyzerSignalSpectrum::MaximumBinCount> rightMagnitudes{};
    std::array<std::complex<double>, consolidator::settings::AnalysisOptions::MaximumFftSize> fftOutput{};
    std::array<std::complex<double>, AnalyzerSignalSpectrum::MaximumBinCount> leftFft{};
};
