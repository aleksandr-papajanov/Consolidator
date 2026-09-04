#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "DspProcessingContext.h"
#include "SharedDspState.h"

namespace consolidator::max::dsp
{

class DspBiquadCascade final
{
public:
    void Prepare(const DspProcessingContext& context) noexcept;
    void Reset() noexcept;
    void SetParameters(
        const FilterSnapshot& parameters,
        bool enabled = true) noexcept;
    void Process(
        DspAudioBuffer& audio,
        DspChannelMode channelMode) noexcept;

    [[nodiscard]] bool RequiresProcessing() const noexcept;

private:
    struct Coefficients
    {
        double b0{ 1.0 };
        double b1{};
        double b2{};
        double a1{};
        double a2{};
    };

    struct State
    {
        double z1{};
        double z2{};
    };

    struct Stage
    {
        Coefficients coefficients{};
        State left{};
        State right{};
    };

    struct Path
    {
        std::array<Stage, 2> stages{};
        std::size_t count{};
    };

    [[nodiscard]] Path Compile(
        const FilterSnapshot& parameters,
        bool enabled) const noexcept;
    [[nodiscard]] static Coefficients CompileBell(
        double frequencyHz,
        double q,
        double gainDb,
        double sampleRate) noexcept;
    [[nodiscard]] static Coefficients CompileShelf(
        bool lowShelf,
        double frequencyHz,
        double q,
        double gainDb,
        double sampleRate) noexcept;
    [[nodiscard]] static double ProcessPath(
        Path& path,
        double input,
        bool rightChannel) noexcept;
    [[nodiscard]] static bool ParametersEqual(
        const FilterSnapshot& left,
        const FilterSnapshot& right) noexcept;
    static void ClearState(Path& path) noexcept;

    double sampleRate_{ 48000.0 };
    std::size_t transitionLength_{ 1 };
    std::size_t transitionPosition_{};
    FilterSnapshot parameters_{};
    Path current_{};
    Path target_{};
    bool enabled_{};
    bool initialized_{};
};

} // namespace consolidator::max::dsp
