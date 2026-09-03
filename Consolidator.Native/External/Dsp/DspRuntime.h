#pragma once

#include <cstddef>
#include <cstdint>

#include "DspRuntimeGraph.h"
#include "DspStateConsumer.h"

namespace consolidator::max::dsp
{

class DspRuntime final
{
public:
    void Prepare(double sampleRate, std::size_t maximumFrameCount);
    void Reset() noexcept;
    void ProcessBlock(
        DspStateExchange& exchange,
        double* mainLeft,
        double* mainRight,
        std::size_t frameCount) noexcept;

private:
    DspRuntimeGraph graph_;
    DspSnapshot snapshot_{};
    std::uint32_t consumerIndex_{};
};

} // namespace consolidator::max::dsp
