#pragma once

#include <cstddef>

#include "SharedDspState.h"

namespace consolidator::max
{

class DspParameterSmoother
{
public:
    DspParameterSmoother() noexcept;

    void Prepare(double sampleRate) noexcept;

    void SetTarget(const DspSnapshot& target) noexcept;

    [[nodiscard]] const DspSnapshot& Advance() noexcept;

    [[nodiscard]] const DspSnapshot& Current() const noexcept;

private:
    static constexpr double kDefaultSampleRate = 48000.0;
    static constexpr double kRampDurationSeconds = 0.01;

    double sampleRate_{kDefaultSampleRate};
    DspSnapshot current_{};
    DspSnapshot target_{};
    DspSnapshot step_{};
    std::size_t remainingSamples_{};
};

} // namespace consolidator::max
