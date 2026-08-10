#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "Core/Settings/DspDeviceSettings.h"
#include "Dsp/Processors/DspDevice.h"

namespace consolidator::dsp
{

struct BiquadCoefficients
{
    double b0 = 1.0;
    double b1 = 0.0;
    double b2 = 0.0;
    double a1 = 0.0;
    double a2 = 0.0;
};

struct FilterMemory
{
    double z1 = 0.0;
    double z2 = 0.0;
};

struct FilterRuntimeState
{
    float frequencyHz = 1000.0f;
    float q = 0.707f;
    float gainDb = 0.0f;
    bool bypass = false;
    BiquadCoefficients coefficients;
    std::array<FilterMemory, 2> channelStates{};
    double sampleRate = core::settings::kDefaultSampleRate;
    std::size_t activeChannelCount = 2;
    bool isNeutral = true;
};

class Filter : public DspDevice
{
public:
    Filter(
        DeviceId deviceId,
        detail::ElementKind elementKind,
        std::uint8_t elementIndex);

    virtual void Prepare(
        double sampleRate,
        std::size_t channelCount);

    virtual void Reset() noexcept;

    void Process(
        const double* input,
        double* output,
        std::size_t frameCount,
        std::size_t channelCount) override;

    bool ApplyParameter(
        const core::StatePath& route,
        const ParameterVariant& value,
        std::size_t depth)
    {
        return DspDevice::ApplyParameter(route, value, depth);
    }


    [[nodiscard]] virtual double ProcessSample(
        double input,
        std::size_t channel) noexcept;

    [[nodiscard]] FilterId GetFilterId() const noexcept
    {
        return detail::ToFilterId(GetElementIndex());
    }

    [[nodiscard]] bool IsNeutral() const noexcept override;

    void CommitRuntimeUpdates() override;

    [[nodiscard]] const FilterRuntimeState& GetRuntimeState() const noexcept
    {
        return runtimeState_;
    }


protected:
    static constexpr std::size_t kMaximumChannelCount = 2;
    virtual void RecalculateCoefficients() = 0;

    [[nodiscard]] virtual bool CalculateIsNeutral() const noexcept;

    void SetNormalizedCoefficients(
        const BiquadCoefficients& coefficients) noexcept
    {
        runtimeState_.coefficients = coefficients;
    }

    [[nodiscard]] double GetSampleRate() const noexcept
    {
        return runtimeState_.sampleRate;
    }

    void InitializeParameters(
        double frequencyHz,
        double q,
        double gainDb) noexcept
    {
        runtimeState_.frequencyHz = static_cast<float>(frequencyHz);
        runtimeState_.q = static_cast<float>(q);
        runtimeState_.gainDb = static_cast<float>(gainDb);
    }

    void RecalculateRuntime() noexcept;

    FilterRuntimeState runtimeState_;

private:
    bool ApplyOwnParameter(
        const core::StatePath& route,
        const ParameterVariant& value) override;


    [[nodiscard]] double ProcessActiveSample(
        double input,
        std::size_t channel) noexcept;

    [[nodiscard]] double GetMaximumFrequencyHz() const noexcept;

};

} // namespace consolidator::dsp
