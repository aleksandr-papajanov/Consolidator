#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>

#include "../Core/AudioBuffer.h"

namespace consolidator::audio::primitives
{

struct ChannelView
{
    double* left{};
    double* right{};
    std::size_t channelCount{};

    [[nodiscard]] double MeanSquare(std::size_t index) const noexcept
    {
        const auto l = left != nullptr ? left[index] : 0.0;
        const auto r = right != nullptr ? right[index] : 0.0;
        return (l * l + r * r) / static_cast<double>(std::max(channelCount, std::size_t{ 1 }));
    }

    [[nodiscard]] double Peak(std::size_t index) const noexcept
    {
        return std::max(left != nullptr ? std::abs(left[index]) : 0.0,
            right != nullptr ? std::abs(right[index]) : 0.0);
    }

    void ApplyGain(std::size_t index, double gain) const noexcept
    {
        if (left != nullptr)
        {
            left[index] *= gain;
        }
        if (right != nullptr)
        {
            right[index] *= gain;
        }
    }
};

[[nodiscard]] inline ChannelView SelectChannels(
    AudioBuffer& audio,
    ChannelMode mode) noexcept
{
    const auto left = mode != ChannelMode::Right ? audio.left : nullptr;
    const auto right = mode != ChannelMode::Left ? audio.right : nullptr;
    return { left, right, static_cast<std::size_t>(left != nullptr) + static_cast<std::size_t>(right != nullptr) };
}

} // namespace consolidator::audio::primitives
