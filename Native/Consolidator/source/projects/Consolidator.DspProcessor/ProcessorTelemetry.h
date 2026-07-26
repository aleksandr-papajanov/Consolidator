#pragma once

#include "Settings/AudioOptions.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

struct ProcessorTelemetryFrame final {
    double compressorReductionDb = 0.0;
    double saturationNonlinearRatio = 0.0;
    double saturationLevelDeltaDb = 0.0;
    double inputPreDb = 0.0;
    double inputPostDb = 0.0;
    double outputPreDb = 0.0;
    double outputPostDb = 0.0;
    double compressorOutputDb = 0.0;
    double saturatorOutputDb = 0.0;
};

class ProcessorTelemetryAccumulator final {
public:
    void ObserveCompressor(double output, double reductionDb) noexcept {
        compressorReductionDb += reductionDb;
        compressorOutputEnergy += output * output;
    }

    void ObserveSaturator(double input, double output) noexcept {
        saturatorInputEnergy += input * input;
        saturatorOutputEnergy += output * output;
        saturatorCrossEnergy += input * output;
    }

    void ObserveInputGain(double input, double output) noexcept {
        inputGainPreEnergy += input * input;
        inputGainPostEnergy += output * output;
    }

    void ObserveOutputGain(double input, double output) noexcept {
        outputGainPreEnergy += input * input;
        outputGainPostEnergy += output * output;
    }

    bool Advance() noexcept {
        return ++sampleCount >= consolidator::settings::AudioOptions::ProcessorTelemetryWindowSamples;
    }

    bool IsSilent() const noexcept {
        if (sampleCount == 0) return true;
        const auto meanSquare = inputGainPreEnergy / (2.0 * static_cast<double>(sampleCount));
        const auto threshold = std::pow(
            10.0,
            consolidator::settings::AudioOptions::SilenceThresholdDb / 10.0);
        return meanSquare < threshold;
    }

    ProcessorTelemetryFrame Finish() noexcept {
        ProcessorTelemetryFrame result;
        result.compressorReductionDb = sampleCount == 0
            ? 0.0
            : compressorReductionDb / (2.0 * static_cast<double>(sampleCount));
        result.saturationLevelDeltaDb = EnergyDeltaDb(
            saturatorInputEnergy, saturatorOutputEnergy);
        result.inputPreDb = EnergyLevelDb(inputGainPreEnergy);
        result.inputPostDb = EnergyLevelDb(inputGainPostEnergy);
        result.outputPreDb = EnergyLevelDb(outputGainPreEnergy);
        result.outputPostDb = EnergyLevelDb(outputGainPostEnergy);
        result.compressorOutputDb = EnergyLevelDb(compressorOutputEnergy);
        result.saturatorOutputDb = EnergyLevelDb(saturatorOutputEnergy);

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
        compressorReductionDb = 0.0;
        compressorOutputEnergy = 0.0;
        saturatorInputEnergy = 0.0;
        saturatorOutputEnergy = 0.0;
        saturatorCrossEnergy = 0.0;
        inputGainPreEnergy = 0.0;
        inputGainPostEnergy = 0.0;
        outputGainPreEnergy = 0.0;
        outputGainPostEnergy = 0.0;
    }

private:
    static double EnergyDeltaDb(double inputEnergy, double outputEnergy) noexcept {
        if (inputEnergy <= MinimumEnergy || outputEnergy <= MinimumEnergy) return 0.0;
        return 10.0 * std::log10(outputEnergy / inputEnergy);
    }

    double EnergyLevelDb(double energy) const noexcept {
        if (energy <= MinimumEnergy || sampleCount == 0) return -120.0;
        return 10.0 * std::log10(energy / (2.0 * static_cast<double>(sampleCount)));
    }

    static constexpr double MinimumEnergy = 1.0e-20;
    std::size_t sampleCount = 0;
    double compressorReductionDb = 0.0;
    double compressorOutputEnergy = 0.0;
    double saturatorInputEnergy = 0.0;
    double saturatorOutputEnergy = 0.0;
    double saturatorCrossEnergy = 0.0;
    double inputGainPreEnergy = 0.0;
    double inputGainPostEnergy = 0.0;
    double outputGainPreEnergy = 0.0;
    double outputGainPostEnergy = 0.0;
};
