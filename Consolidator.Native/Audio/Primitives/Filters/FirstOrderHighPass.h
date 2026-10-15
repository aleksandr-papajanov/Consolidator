#pragma once

#include <algorithm>
#include <cmath>
#include <numbers>

namespace consolidator::audio::primitives::filters
{

class FirstOrderHighPass final
{
public:
    void Prepare(double frequencyHz, double sampleRate) noexcept
    {
        const auto warpedFrequency = std::tan(
            std::numbers::pi * std::min(frequencyHz, sampleRate * 0.499) / sampleRate);
        feedforward_ = 1.0 / (1.0 + warpedFrequency);
        feedback_ = (1.0 - warpedFrequency) * feedforward_;
    }

    void Reset() noexcept
    {
        previousInput_ = 0.0;
        previousOutput_ = 0.0;
    }

    [[nodiscard]] double Process(double input) noexcept
    {
        const auto output = feedforward_ * (input - previousInput_) + feedback_ * previousOutput_;
        previousInput_ = input;
        previousOutput_ = output;
        return output;
    }

private:
    double feedforward_{};
    double feedback_{};
    double previousInput_{};
    double previousOutput_{};
};

} // namespace consolidator::audio::primitives::filters
