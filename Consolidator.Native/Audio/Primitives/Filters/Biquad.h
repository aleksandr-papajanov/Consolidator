#pragma once

#include <cmath>

namespace consolidator::audio::primitives::filters
{

struct BiquadCoefficients
{
    double b0{ 1.0 };
    double b1{};
    double b2{};
    double a1{};
    double a2{};
};

// One channel's transposed direct-form II history. Coefficients can be shared;
// this state must remain independent for every channel and every filter path.
struct BiquadState
{
    double z1{};
    double z2{};

    [[nodiscard]] double Process(double input, const BiquadCoefficients& coefficients) noexcept
    {
        const auto output = coefficients.b0 * input + z1;
        z1 = coefficients.b1 * input - coefficients.a1 * output + z2;
        z2 = coefficients.b2 * input - coefficients.a2 * output;
        return output;
    }
};

struct BiquadFilter
{
    BiquadCoefficients coefficients{};
    BiquadState left{};
    BiquadState right{};

    [[nodiscard]] double Process(double input, bool rightChannel) noexcept
    {
        auto& state = rightChannel ? right : left;
        const auto output = state.Process(input, coefficients);
        state.z1 = SanitizeState(state.z1);
        state.z2 = SanitizeState(state.z2);
        return output;
    }

    void Reset() noexcept
    {
        left = {};
        right = {};
    }

private:
    [[nodiscard]] static double SanitizeState(double value) noexcept
    {
        return std::abs(value) < 1.0e-24 ? 0.0 : value;
    }
};

[[nodiscard]] BiquadCoefficients DesignHighPass(
    double frequencyHz, double q, double sampleRate) noexcept;
[[nodiscard]] BiquadCoefficients DesignBell(
    double frequencyHz, double q, double gainDb, double sampleRate) noexcept;
[[nodiscard]] BiquadCoefficients DesignShelf(
    bool lowShelf, double frequencyHz, double q, double gainDb, double sampleRate) noexcept;

} // namespace consolidator::audio::primitives::filters
