#pragma once

#include "Settings/AudioOptions.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

struct ProcessorTelemetryFrame final {
    double compressorReductionDb = 0.0;
    double saturationNonlinearRatio = 0.0;
    double saturationLevelDeltaDb = 0.0;
};

class ProcessorTelemetryAccumulator final {
public:
    void ObserveCompressor(double input, double output) noexcept {
        compressorInputEnergy += input * input;
        compressorOutputEnergy += output * output;
    }

    void ObserveSaturator(double input, double output) noexcept {
        saturatorInputEnergy += input * input;
        saturatorOutputEnergy += output * output;
        saturatorCrossEnergy += input * output;
    }

    bool Advance() noexcept {
        return ++sampleCount >= consolidator::settings::AudioOptions::ProcessorTelemetryWindowSamples;
    }

    ProcessorTelemetryFrame Finish() noexcept {
        ProcessorTelemetryFrame result;
        result.compressorReductionDb = std::min(
            0.0, EnergyDeltaDb(compressorInputEnergy, compressorOutputEnergy));
        result.saturationLevelDeltaDb = EnergyDeltaDb(
            saturatorInputEnergy, saturatorOutputEnergy);

        if (saturatorInputEnergy > MinimumEnergy && saturatorOutputEnergy > MinimumEnergy) {
            const auto linearGain = saturatorCrossEnergy / saturatorInputEnergy;
            const auto residualEnergy = std::max(
                0.0,
                saturatorOutputEnergy - 2.0 * linearGain * saturatorCrossEnergy +
                    linearGain * linearGain * saturatorInputEnergy);
            result.saturationNonlinearRatio = std::clamp(
                std::sqrt(residualEnergy / saturatorOutputEnergy), 0.0, 1.0);
        }

        Reset();
        return result;
    }

    void Reset() noexcept {
        sampleCount = 0;
        compressorInputEnergy = 0.0;
        compressorOutputEnergy = 0.0;
        saturatorInputEnergy = 0.0;
        saturatorOutputEnergy = 0.0;
        saturatorCrossEnergy = 0.0;
    }

private:
    static double EnergyDeltaDb(double inputEnergy, double outputEnergy) noexcept {
        if (inputEnergy <= MinimumEnergy || outputEnergy <= MinimumEnergy) return 0.0;
        return 10.0 * std::log10(outputEnergy / inputEnergy);
    }

    static constexpr double MinimumEnergy = 1.0e-20;
    std::size_t sampleCount = 0;
    double compressorInputEnergy = 0.0;
    double compressorOutputEnergy = 0.0;
    double saturatorInputEnergy = 0.0;
    double saturatorOutputEnergy = 0.0;
    double saturatorCrossEnergy = 0.0;
};
