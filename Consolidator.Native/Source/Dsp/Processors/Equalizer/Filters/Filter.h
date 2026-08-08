#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "Core/Settings/DspDeviceSettings.h"
#include "Dsp/Parameters/DspParameter.h"
#include "Dsp/Processors/DspDevice.h"

namespace consolidator::dsp
{

struct FilterState
{
    DspParameter<float> frequencyHz{
        ParameterId::Frequency,
        static_cast<float>(core::settings::FilterDefaults::kDefaultFrequencyHz),
        static_cast<float>(core::settings::FilterDefaults::kMinFrequencyHz),
        static_cast<float>(core::settings::FilterDefaults::kMaxFrequencyHz)};
    DspParameter<float> q{
        ParameterId::Q,
        static_cast<float>(core::settings::FilterDefaults::kDefaultQ),
        static_cast<float>(core::settings::FilterDefaults::kMinQ),
        static_cast<float>(core::settings::FilterDefaults::kMaxQ)};
    DspParameter<float> gainDb{
        ParameterId::Gain,
        static_cast<float>(core::settings::FilterDefaults::kDefaultGainDb),
        static_cast<float>(core::settings::FilterDefaults::kMinGainDb),
        static_cast<float>(core::settings::FilterDefaults::kMaxGainDb)};
    DspParameter<bool> bypass{ParameterId::Bypass, false};
};

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


    [[nodiscard]] virtual double ProcessSample(
        double input,
        std::size_t channel) noexcept;

    [[nodiscard]] FilterId GetFilterId() const noexcept
    {
        return detail::ToFilterId(GetElementIndex());
    }

    [[nodiscard]] bool IsNeutral() const noexcept override;

    [[nodiscard]] virtual const FilterState& GetState() const noexcept
    {
        return state_;
    }

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
        state_.frequencyHz = frequencyHz;
        state_.q = q;
        state_.gainDb = gainDb;
    }

    void RecalculateRuntime() noexcept;

    FilterState state_{};
    FilterRuntimeState runtimeState_;

private:
    bool ApplyStateParameter(
        const ParameterRoute& route,
        const ParameterValue& value) override;


    [[nodiscard]] double ProcessActiveSample(
        double input,
        std::size_t channel) noexcept;

    [[nodiscard]] double GetMaximumFrequencyHz() const noexcept;

};

} // namespace consolidator::dsp
