#pragma once

#include "DSP/DspChainBuilder.h"
#include "DSP/Dynamics/Compressor.h"
#include "DSP/Distortion/Saturator.h"
#include "DSP/Eq/EqRuntime.h"
#include "DSP/Gain/Gain.h"
#include "DSP/TypedDspDeviceFactory.h"
#include "Snapshots/Snapshots.h"
#include "Definitions/Definitions.h"
#include "Settings/AudioOptions.h"

#include <memory>
#include <algorithm>
#include <string>
#include <vector>

namespace consolidator::dspcore {

class DeviceDspRuntime final {
public:
    void SetSnapshot(domain::DspSnapshot snapshot) { this->snapshot = std::move(snapshot); }

    bool UpdateEqParameter(
        dsp::StereoDspChain& chain,
        long bankId,
        long filterId,
        const std::string& parameter,
        double value,
        double sampleRate
    ) {
        auto* bank = snapshot.eq.FindBank(bankId);
        auto* filter = bank ? bank->FindFilter(filterId) : nullptr;
        const auto definition = domain::FilterDefinitions().find(filterId);
        if (!filter || definition == domain::FilterDefinitions().end()) return false;
        const auto parameterIndex = definition->second.ParameterIndex(parameter);
        if (!parameterIndex || *parameterIndex >= filter->values.size()) return false;
        filter->values[*parameterIndex] = value;

        const auto filterIndex = std::distance(
            bank->filters.begin(),
            std::find_if(bank->filters.begin(), bank->filters.end(),
                [filterId](const auto& candidate) { return candidate.filterId == filterId; })
        );
        if (filterIndex < 0 || filterIndex >= static_cast<long>(bank->filters.size())) return false;
        dsp::EqFilterFactory factory{ definition->second, filter->values, sampleRate };
        return chain.UpdateDevice(
            std::to_string(bankId) + ":" + std::to_string(filterIndex), factory,
            snapshot.eq.IsBypassed() || filter->bypass ||
                std::abs(definition->second.Value(filter->values, "gain", 0.0)) < 1.0e-12);
    }

    bool UpdateGain(
        dsp::StereoDspChain& chain,
        bool input,
        double value,
        double sampleRate
    ) {
        auto& state = input ? snapshot.processor.inputGain : snapshot.processor.outputGain;
        state.gainDb = value;
        dsp::TypedDspDeviceFactory<dsp::Gain, dsp::GainSettings> factory{
            dsp::GainSettings{ state.gainDb, sampleRate }
        };
        return chain.UpdateDevice(input ? "input_gain" : "output_gain", factory);
    }

    bool UpdateCompressorParameter(
        dsp::StereoDspChain& chain,
        const std::string& parameter,
        double value,
        double sampleRate
    ) {
        auto& state = snapshot.processor.compressor;
        if (parameter == "attack") state.attackMs = value;
        else if (parameter == "release") state.releaseMs = value;
        else if (parameter == "threshold") state.thresholdDb = value;
        else if (parameter == "output") state.outputDb = value;
        else if (parameter == "mix") state.mix = value;
        else return false;
        return chain.UpdateDevice("compressor", CompressorFactory(sampleRate));
    }

    bool UpdateSaturatorParameter(
        dsp::StereoDspChain& chain,
        const std::string& parameter,
        double value,
        double sampleRate
    ) {
        auto& state = snapshot.processor.saturator;
        if (parameter == "saturation") state.saturation = value;
        else if (parameter == "output") state.outputDb = value;
        else return false;
        return chain.UpdateDevice("saturator", SaturatorFactory(sampleRate));
    }

    std::vector<dsp::DspDeviceRegistration> BuildRegistrations(double sampleRate) const {
        dsp::EqRuntime eqRuntime;
        eqRuntime.SetSnapshot(snapshot.eq);
        dsp::DspChainBuilder builder;
        auto order = 0L;
        const auto smoothingSamples = settings::AudioOptions::ParameterSmoothingSamples(sampleRate);

        const auto inputGain = snapshot.processor.inputGain;
        builder.UpsertDevice({
            "input_gain",
            std::make_shared<dsp::TypedDspDeviceFactory<dsp::Gain, dsp::GainSettings>>(
                dsp::GainSettings{ inputGain.gainDb, sampleRate }),
            false,
            order++,
            smoothingSamples
        });

        const auto saturator = snapshot.processor.saturator;
        builder.UpsertDevice({
            "saturator",
            std::make_shared<dsp::TypedDspDeviceFactory<dsp::Saturator, dsp::SaturatorSettings>>(
                dsp::SaturatorSettings{
                    saturator.saturation,
                    saturator.outputDb,
                    saturator.detectorFilters,
                    saturator.detectorListen,
                    sampleRate
                }),
            saturator.bypass,
            order++,
            smoothingSamples
        });

        const auto compressor = snapshot.processor.compressor;
        builder.UpsertDevice({
            "compressor",
            std::make_shared<dsp::TypedDspDeviceFactory<dsp::Compressor, dsp::CompressorSettings>>(
                dsp::CompressorSettings{
                    compressor.attackMs,
                    compressor.releaseMs,
                    compressor.thresholdDb,
                    compressor.outputDb,
                    compressor.mix,
                    compressor.detectorFilters,
                    compressor.detectorListen,
                    sampleRate
                }),
            compressor.bypass,
            order++,
            smoothingSamples
        });

        order = eqRuntime.AddAllBanks(builder, sampleRate, order);

        const auto outputGain = snapshot.processor.outputGain;
        builder.UpsertDevice({
            "output_gain",
            std::make_shared<dsp::TypedDspDeviceFactory<dsp::Gain, dsp::GainSettings>>(
                dsp::GainSettings{ outputGain.gainDb, sampleRate }),
            false,
            order++,
            smoothingSamples
        });
        return builder.TakeDevices();
    }

    dsp::StereoDspChain BuildStereo(double sampleRate) const {
        dsp::DspChainBuilder builder;
        builder.SetDevices(BuildRegistrations(sampleRate));
        return builder.BuildStereo();
    }

private:
    dsp::TypedDspDeviceFactory<dsp::Compressor, dsp::CompressorSettings>
    CompressorFactory(double sampleRate) const {
        const auto& state = snapshot.processor.compressor;
        return dsp::TypedDspDeviceFactory<dsp::Compressor, dsp::CompressorSettings>{
            dsp::CompressorSettings{
            state.attackMs, state.releaseMs, state.thresholdDb, state.outputDb,
            state.mix, state.detectorFilters, state.detectorListen, sampleRate
        } };
    }

    dsp::TypedDspDeviceFactory<dsp::Saturator, dsp::SaturatorSettings>
    SaturatorFactory(double sampleRate) const {
        const auto& state = snapshot.processor.saturator;
        return dsp::TypedDspDeviceFactory<dsp::Saturator, dsp::SaturatorSettings>{
            dsp::SaturatorSettings{
            state.saturation, state.outputDb, state.detectorFilters,
            state.detectorListen, sampleRate
        } };
    }

    domain::DspSnapshot snapshot;
};

} // namespace consolidator::dspcore
