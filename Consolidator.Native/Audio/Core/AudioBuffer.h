#pragma once

#include <cstddef>

namespace consolidator::audio
{

enum class ChannelMode
{
    Stereo,
    Left,
    Right
};

// Borrowed storage, valid only for the current audio callback. Null lanes are absent channels.
struct AudioBuffer
{
    double* left{};
    double* right{};
    std::size_t frameCount{};
};

// Read-only detector audio has the same sample count as its corresponding main buffer.
struct DetectorBuffer
{
    const double* left{};
    const double* right{};
};

// Prepare runs while audio is stopped. Every allocation must fit this bounded capacity.
struct ProcessingContext
{
    double sampleRate{};
    std::size_t maximumFrameCount{};
};

} // namespace consolidator::audio
