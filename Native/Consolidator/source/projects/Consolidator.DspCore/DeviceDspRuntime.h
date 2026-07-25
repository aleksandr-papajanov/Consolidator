#pragma once

#include "DSP/DspChainBuilder.h"
#include "DSP/Dynamics/Compressor.h"
#include "DSP/Distortion/Saturator.h"
#include "DSP/Eq/EqRuntime.h"
#include "DSP/Gain/Gain.h"
#include "DSP/TypedDspDeviceFactory.h"
#include "Snapshots/Snapshots.h"
#include "Settings/AudioOptions.h"

#include <memory>
#include <vector>

namespace consolidator::dspcore {

class DeviceDspRuntime final {
public:
    void SetSnapshot(domain::DspSnapshot snapshot) { this->snapshot = std::move(snapshot); }

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
                    saturator.inputDb,
                    saturator.outputDb,
                    saturator.mix,
                    saturator.mode,
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
                    compressor.inputDb,
                    compressor.outputDb,
                    compressor.mix,
                    compressor.mode,
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
    domain::DspSnapshot snapshot;
};

} // namespace consolidator::dspcore
