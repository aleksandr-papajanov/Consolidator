#pragma once

#include <cstddef>

#include "../Chain/ConsolidatorChain.h"
#include "../Primitives/RmsMeter.h"
#include "../../Transport/SharedDspState.h"

namespace consolidator::audio::runtime
{

// One host instance owns one runtime. Prepare/teardown require stopped audio;
// Reset, snapshot consumption and ProcessBlock belong to the audio owner.
class AudioRuntime final
{
public:
    void Prepare(double sampleRate, std::size_t maximumFrameCount);
    void Reset() noexcept;
    void ProcessBlock(transport::DspStateExchange& exchange,
        double* left, double* right, std::size_t frameCount) noexcept;

private:
    void PublishTelemetry(transport::DspTelemetryExchange& exchange, bool processed) noexcept;

    ProcessingContext context_{};
    ConsolidatorChain chain_;
    primitives::RmsMeter outputMeter_;
    transport::DspSnapshot snapshot_{};
    std::uint32_t consumerIndex_{};
    bool hasSnapshot_{};
    bool prepared_{};
};

} // namespace consolidator::audio::runtime
