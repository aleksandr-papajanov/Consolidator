#pragma once

#include "../Core/AudioBuffer.h"

namespace consolidator::audio::primitives
{

// Linked mean-square meter. It measures audio without changing it.
class RmsMeter final
{
public:
    void Prepare(const ProcessingContext& context) noexcept;
    void Reset() noexcept;
    void Process(AudioBuffer audio, ChannelMode mode) noexcept;
    [[nodiscard]] double LevelDb() const noexcept;

private:
    double coefficient_{};
    double energy_{};
};

} // namespace consolidator::audio::primitives
