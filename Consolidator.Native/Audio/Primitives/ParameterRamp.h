#pragma once

#include <cstddef>

#include "../Core/AudioBuffer.h"

namespace consolidator::audio::primitives
{

class ParameterRamp final
{
public:
    explicit ParameterRamp(double initialValue = 0.0) noexcept;

    void Prepare(const ProcessingContext& context) noexcept;
    void Reset() noexcept;
    void SetTarget(double targetValue) noexcept;
    [[nodiscard]] double NextValue() noexcept
    {
        const auto value = currentValue_;
        Advance(1);
        return value;
    }

    void Advance(std::size_t sampleCount) noexcept
    {
        if (remainingSamples_ == 0 || sampleCount == 0)
        {
            return;
        }
        if (sampleCount >= remainingSamples_)
        {
            currentValue_ = targetValue_;
            step_ = 0.0;
            remainingSamples_ = 0;
            return;
        }
        currentValue_ += step_ * static_cast<double>(sampleCount);
        remainingSamples_ -= sampleCount;
    }

    [[nodiscard]] double CurrentValue() const noexcept
    {
        return currentValue_;
    }

    [[nodiscard]] double TargetValue() const noexcept
    {
        return targetValue_;
    }

    [[nodiscard]] bool IsSmoothing() const noexcept
    {
        return remainingSamples_ > 0;
    }

private:
    double currentValue_{};
    double targetValue_{};
    double step_{};
    std::size_t rampSampleCount_{ 1 };
    std::size_t remainingSamples_{};
};

} // namespace consolidator::audio::primitives
