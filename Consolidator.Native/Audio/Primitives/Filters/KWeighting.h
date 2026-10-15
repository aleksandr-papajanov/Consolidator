#pragma once

#include "Biquad.h"

namespace consolidator::audio::primitives::filters
{

// BS.1770 frequency weighting only. Time averaging, channel aggregation and gating
// belong to the consumer; this filter alone does not constitute a LUFS meter.
class KWeighting final
{
public:
    void Prepare(double sampleRate) noexcept;
    void Reset() noexcept;
    [[nodiscard]] double Process(double input, bool rightChannel) noexcept
    {
        return highPass_.Process(shelf_.Process(input, rightChannel), rightChannel);
    }

private:
    BiquadFilter shelf_;
    BiquadFilter highPass_;
};

} // namespace consolidator::audio::primitives::filters
