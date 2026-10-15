#pragma once

#include "../../Primitives/ParameterRamp.h"
#include "../../Core/AudioBuffer.h"

namespace consolidator::audio::algorithms
{

class LimiterProcessor final
{
public:
    void Prepare(const ProcessingContext& context) noexcept;
    void Reset() noexcept;
    void SetControls(double ceilingDb, bool enabled) noexcept;
    void Process(AudioBuffer audio, ChannelMode mode) noexcept;
    [[nodiscard]] double ReductionDb() const noexcept;
    [[nodiscard]] bool RequiresProcessing() const noexcept;

private:
    double reductionDb_{};
    primitives::ParameterRamp targetDb_{ -1.0 };
    double gain_{ 1.0 };
    double releaseCoefficient_{};
    bool enabled_{};
};

} // namespace consolidator::audio::algorithms
