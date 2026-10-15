#pragma once

#include <array>
#include <vector>

#include "../Primitives/Filters/BiquadCascade.h"

namespace consolidator::audio::devices
{

// The device owns this private signal. Its filters never modify the audible main path.
class DetectorEqualizer final
{
public:
    void Prepare(const ProcessingContext& context);
    void Reset() noexcept;
    void SetParameters(const std::array<FilterParameters, 2>& filters) noexcept;
    [[nodiscard]] DetectorBuffer Capture(AudioBuffer audio, ChannelMode mode) noexcept;

private:
    std::array<primitives::filters::BiquadCascade, 2> filters_{};
    std::vector<double> left_;
    std::vector<double> right_;
};

} // namespace consolidator::audio::devices
