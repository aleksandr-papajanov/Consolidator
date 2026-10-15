#pragma once

#include "../Devices/CompressorDevice.h"
#include "../Devices/EqualizerDevice.h"
#include "../Devices/InputDevice.h"
#include "../Devices/OutputDevice.h"
#include "../Devices/PolishDevice.h"
#include "../Devices/SaturatorDevice.h"
#include "../Devices/ShaperDevice.h"

namespace consolidator::audio
{

struct ChainParameters
{
    devices::InputParameters input;
    devices::SaturatorParameters saturator;
    devices::ShaperParameters shaper;
    devices::CompressorParameters compressor;
    devices::EqualizerParameters equalizer;
    devices::PolishParameters polish;
    devices::OutputParameters output;
};

struct ChainTelemetry
{
    devices::GainStageTelemetry input;
    devices::GainStageTelemetry output;
    double compressorReductionDb{};
    double limiterReductionDb{};
};

// Audio-thread owned. Parameters change targets; they never replace the chain or its history.
class ConsolidatorChain final
{
public:
    void Prepare(const ProcessingContext& context);
    void Reset() noexcept;
    void SetParameters(const ChainParameters& parameters) noexcept;
    void Process(AudioBuffer audio, ChannelMode mode) noexcept;
    [[nodiscard]] ChainTelemetry Telemetry() const noexcept;

private:
    devices::InputDevice input_;
    devices::SaturatorDevice saturator_;
    devices::ShaperDevice shaper_;
    devices::CompressorDevice compressor_;
    devices::EqualizerDevice equalizer_;
    devices::PolishDevice polish_;
    devices::OutputDevice output_;
};

} // namespace consolidator::audio
