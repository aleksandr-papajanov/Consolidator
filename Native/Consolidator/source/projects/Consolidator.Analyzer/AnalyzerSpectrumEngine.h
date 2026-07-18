#pragma once

#include "AnalyzerFrameBuffer.h"
#include "AnalyzerCurveBatch.h"
#include "DSP/Spectrum/FftEngine.h"
#include "Settings/GlobalSettings.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <vector>

class AnalyzerSpectrumEngine {
public:
    void SetSampleRate(double sampleRate) {
        this->sampleRate = sampleRate;
    }

    void Analyze(
        const AnalyzerFrameBuffer& frame,
        AnalyzerCurveBatch& curves
    ) const {
        const int fftSize = static_cast<int>(consolidator::settings::GlobalSettings::DefaultFftSize);
        const int binsOut = static_cast<int>(consolidator::settings::GlobalSettings::DefaultCurvePointCount);
        const auto referenceSpectrum = StereoMagnitudeDb(
            frame.ReferenceLeft(),
            frame.ReferenceRight(),
            frame.WriteIndex(),
            fftSize);

        const auto currentSpectrum = StereoMagnitudeDb(
            frame.CurrentLeft(),
            frame.CurrentRight(),
            frame.WriteIndex(),
            fftSize);
        const int previousPendingCount = curves.Prepare();

        for (int i = 0; i < binsOut; ++i) {
            const int sourceIndex = MapOutputBinToFftBin(i, binsOut, fftSize);

            curves.StoreBin(
                i,
                previousPendingCount,
                currentSpectrum[sourceIndex],
                referenceSpectrum[sourceIndex]);
        }

        curves.FinalizeFrame();
    }

private:
    std::vector<double> StereoMagnitudeDb(
        const std::array<double, consolidator::settings::GlobalSettings::MaximumFftSize>& left,
        const std::array<double, consolidator::settings::GlobalSettings::MaximumFftSize>& right,
        int writeIndex,
        int fftSize
    ) const {
        const auto leftDb = MagnitudeDb(MakeWindowedCopy(left, writeIndex, fftSize));
        const auto rightDb = MagnitudeDb(MakeWindowedCopy(right, writeIndex, fftSize));

        std::vector<double> stereoDb(fftSize / 2);

        for (int i = 0; i < fftSize / 2; ++i) {
            stereoDb[i] = consolidator::audio::StereoSample::FromDecibels(
                leftDb[i], rightDb[i]).MagnitudeDb();
        }

        return stereoDb;
    }

    std::vector<double> MakeWindowedCopy(
        const std::array<double, consolidator::settings::GlobalSettings::MaximumFftSize>& source,
        int writeIndex,
        int fftSize
    ) const {
        std::vector<double> output(fftSize);
        const int start = writeIndex;

        for (int i = 0; i < fftSize; ++i) {
            const int index = (start + i) % fftSize;
            const double hann = consolidator::settings::GlobalSettings::HannWindowCoefficient * (1.0 - std::cos(
                (2.0 * std::numbers::pi * i) / (fftSize - 1)));

            output[i] = source[index] * hann;
        }

        return output;
    }

    std::vector<double> MagnitudeDb(const std::vector<double>& input) const {
        const consolidator::dsp::FftEngine engine;
        const auto output = engine.Forward(input);

        const int fftSize = static_cast<int>(engine.Size());
        std::vector<double> decibels(fftSize / 2);
        const double coherentGain = consolidator::settings::GlobalSettings::HannWindowCoherentGain;
        const double amplitudeScale = static_cast<double>(fftSize) * coherentGain *
            consolidator::settings::GlobalSettings::SingleSidedSpectrumScale;

        for (int i = 0; i < fftSize / 2; ++i) {
            const double re = output[static_cast<std::size_t>(i)].real();
            const double im = output[static_cast<std::size_t>(i)].imag();
            const double magnitude = std::sqrt(re * re + im * im);

            decibels[i] = consolidator::helpers::NumericHelper::MagnitudeToDecibels(
                magnitude / amplitudeScale);
        }

        return decibels;
    }

    int MapOutputBinToFftBin(int index, int binsOut, int fftSize) const {
        if (binsOut <= 1) {
            return 0;
        }

        const double normalized = static_cast<double>(index) / static_cast<double>(binsOut - 1);
        const int maxBin = (fftSize / 2) - 1;
        const double nyquist = sampleRate * 0.5;
        const double maximumFrequency = std::max(
            consolidator::settings::GlobalSettings::MinimumFrequencyHz,
            std::min(consolidator::settings::GlobalSettings::MaximumFrequencyHz, nyquist));
        const double frequencyHz = consolidator::settings::GlobalSettings::MinimumFrequencyHz *
            std::pow(maximumFrequency /
                consolidator::settings::GlobalSettings::MinimumFrequencyHz, normalized);
        const double mappedBin = frequencyHz * static_cast<double>(fftSize) / sampleRate;

        return std::clamp(static_cast<int>(std::round(mappedBin)), 1, maxBin);
    }

    double sampleRate = consolidator::settings::GlobalSettings::DefaultSampleRateHz;
};
