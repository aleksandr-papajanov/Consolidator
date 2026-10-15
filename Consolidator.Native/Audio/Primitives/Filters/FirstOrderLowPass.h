#pragma once

#include <cmath>
#include <numbers>

namespace consolidator::audio::primitives::filters
{

// Exponential one-pole section. Unlike the TPT sections in OnePole.h, this is
// the existing Saturation crossover law; substituting one for the other changes the response.
class FirstOrderLowPass final
{
public:
    void Prepare(double frequencyHz, double sampleRate) noexcept
    {
        coefficient_ = 1.0 - std::exp(-2.0 * std::numbers::pi * frequencyHz / sampleRate);
    }

    void Reset() noexcept
    {
        state_ = 0.0;
    }

    [[nodiscard]] double Process(double input) noexcept
    {
        state_ += coefficient_ * (input - state_);
        return state_;
    }

private:
    double coefficient_{};
    double state_{};
};

} // namespace consolidator::audio::primitives::filters
