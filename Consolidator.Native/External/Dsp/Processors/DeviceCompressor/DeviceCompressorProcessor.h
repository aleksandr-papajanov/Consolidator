#pragma once

#include "AdaptiveCompressorGain.h"
#include "../Cla2a/Cla2aProcessor.h"
#include "../Cla3a/Cla3aProcessor.h"
#include "../Cla76/Cla76Processor.h"
#include "../SslChannel/SslChannelProcessor.h"

namespace consolidator::max::dsp::processors
{

enum class CompressorAlgorithm
{
    Cla76,
    Cla3a,
    Cla2a,
    Ssl
};

struct DeviceCompressorControls : CompressorControls
{
    CompressorAlgorithm algorithm{ CompressorAlgorithm::Cla76 };
    double attack{ 2.0 / 3.0 };
    double release{ 0.5 };
    double ratio{ kNormalRatioDefault };
    bool autoGain{ true };
};

// Owns all prepared models, but processes only the selected one. Normal controls only.
class DeviceCompressorProcessor final
{
public:
    void Prepare(const DspProcessingContext& context);
    void Reset() noexcept;
    void SetControls(const DeviceCompressorControls& controls) noexcept;
    [[nodiscard]] double OutputDb() const noexcept;
    void Process(DspAudioBuffer audio, DspChannelMode mode = DspChannelMode::Stereo) noexcept;

private:
    void ApplyActiveControls() noexcept;
    void ResetActiveProcessor() noexcept;

    DeviceCompressorControls controls_;
    bool resetPending_{};
    AdaptiveCompressorGain adaptiveGain_;
    Cla76Processor cla76_;
    Cla3aProcessor cla3a_;
    Cla2aProcessor cla2a_;
    SslChannelProcessor ssl_;
};

} // namespace consolidator::max::dsp::processors
