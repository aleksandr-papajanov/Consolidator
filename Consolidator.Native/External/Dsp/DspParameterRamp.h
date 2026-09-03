#pragma once

#include <cstddef>

#include "DspProcessingContext.h"

namespace consolidator::max::dsp
{

class DspParameterRamp final
{
public:
    explicit DspParameterRamp(double initialValue = 0.0) noexcept;

    void Prepare(const DspProcessingContext& context) noexcept;
    void Reset() noexcept;
    void SetTarget(double targetValue) noexcept;
    [[nodiscard]] double NextValue() noexcept;
    void Advance(std::size_t sampleCount) noexcept;

    [[nodiscard]] double CurrentValue() const noexcept;
    [[nodiscard]] double TargetValue() const noexcept;
    [[nodiscard]] bool IsSmoothing() const noexcept;

private:
    double currentValue_{};
    double targetValue_{};
    double step_{};
    std::size_t rampSampleCount_{ 1 };
    std::size_t remainingSamples_{};
};

} // namespace consolidator::max::dsp
