#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "Biquad.h"
#include "../../Core/AudioBuffer.h"
#include "../../Core/FilterParameters.h"

namespace consolidator::audio::primitives::filters
{

class BiquadCascade final
{
public:
    void Prepare(const ProcessingContext& context) noexcept;
    void Reset() noexcept;
    void SetParameters(
        const FilterParameters& parameters,
        bool enabled = true) noexcept;
    void Process(
        AudioBuffer& audio,
        ChannelMode channelMode) noexcept;

    [[nodiscard]] bool RequiresProcessing() const noexcept;

private:
    struct Path
    {
        std::array<BiquadFilter, 2> stages{};
        std::size_t count{};
    };

    [[nodiscard]] Path Compile(
        const FilterParameters& parameters,
        bool enabled) const noexcept;
    [[nodiscard]] static double ProcessPath(
        Path& path,
        double input,
        bool rightChannel) noexcept;
    [[nodiscard]] static bool ParametersEqual(
        const FilterParameters& left,
        const FilterParameters& right) noexcept;
    static void ClearState(Path& path) noexcept;

    double sampleRate_{ 48000.0 };
    std::size_t transitionLength_{ 1 };
    std::size_t transitionPosition_{};
    FilterParameters parameters_{};
    Path current_{};
    Path target_{};
    bool enabled_{};
    bool initialized_{};
};

} // namespace consolidator::audio::primitives::filters
