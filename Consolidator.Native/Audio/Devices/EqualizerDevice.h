#pragma once

#include <array>

#include "../Primitives/Filters/BiquadCascade.h"

namespace consolidator::audio::devices
{

struct EqualizerParameters
{
    std::array<FilterParameters, 49> filters{};
    std::array<bool, 7> banks{};
    bool bypass{};
};

class EqualizerDevice final
{
public:
    void Prepare(const ProcessingContext& context) noexcept;
    void Reset() noexcept;
    void SetParameters(const EqualizerParameters& parameters) noexcept;
    void Process(AudioBuffer audio, ChannelMode mode) noexcept;

private:
    std::array<primitives::filters::BiquadCascade, 49> filters_{};
};

} // namespace consolidator::audio::devices
