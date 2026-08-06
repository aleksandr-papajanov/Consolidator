#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "Core/Settings/DspDeviceSettings.h"
#include "Dsp/Parameters/ParameterAddress.h"
#include "Dsp/Parameters/ParameterChange.h"
#include "Dsp/Processors/IDspDevice.h"

namespace consolidator::dsp
{

struct FilterParameters
{
    double frequencyHz = core::settings::FilterDefaults::kDefaultFrequencyHz;
    double q = core::settings::FilterDefaults::kDefaultQ;
    double gainDb = core::settings::FilterDefaults::kDefaultGainDb;
    bool bypass = false;
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

class Filter : public IDspDevice
{
public:
    Filter(DeviceId deviceId, detail::ElementKind elementKind, std::uint8_t elementIndex);

    virtual void Prepare(double sampleRate, std::size_t channelCount);
    virtual void Reset() noexcept;

    void Process(const double* input, double* output, std::size_t frameCount, std::size_t channelCount) override;
    void ApplyParameterChange(const ParameterChange& change) override;

    [[nodiscard]] DeviceId GetDeviceId() const noexcept override { return deviceId_; }
    [[nodiscard]] detail::ElementKind GetElementKind() const noexcept override { return elementKind_; }
    [[nodiscard]] std::uint8_t GetElementIndex() const noexcept override { return elementIndex_; }

    [[nodiscard]] virtual double ProcessSample(double input, std::size_t channel) noexcept;

    [[nodiscard]] const FilterParameters& GetParameters() const noexcept { return parameters_; }
    [[nodiscard]] EqFilterId GetEqFilterId() const noexcept { return detail::ToEqFilterId(elementIndex_); }
    [[nodiscard]] BankId GetBankId() const noexcept { return bankId_; }

    [[nodiscard]] bool IsNeutral() const noexcept override { return isNeutral_; }

    DeviceId deviceId_;
    BankId bankId_ = BankId::Bank0;

protected:
    static constexpr std::size_t kMaximumChannelCount = 2;
    static constexpr double kMinimumFrequencyHz = 20.0;
    static constexpr double kMinimumQ = 0.1;

    virtual void RecalculateCoefficients() = 0;

    void SetNormalizedCoefficients(const BiquadCoefficients& c) noexcept { coefficients_ = c; }
    [[nodiscard]] double GetSampleRate() const noexcept { return sampleRate_; }

    void InitializeParameters(double f, double q, double g) noexcept
    { parameters_.frequencyHz = f; parameters_.q = q; parameters_.gainDb = g; }

    void SetNeutral(bool v) noexcept { isNeutral_ = v; }

    detail::ElementKind elementKind_;
    std::uint8_t elementIndex_;
    FilterParameters parameters_;
    BiquadCoefficients coefficients_;
    std::array<FilterMemory, kMaximumChannelCount> channelStates_{};
    double sampleRate_ = core::settings::kDefaultSampleRate;
    std::size_t activeChannelCount_ = kMaximumChannelCount;

private:
    void SetFrequency(float f);
    void SetQ(float q);
    void SetGain(float gainDb);
    void SetBypass(bool bypass) noexcept;

    [[nodiscard]] double ProcessActiveSample(double input, std::size_t channel) noexcept;
    [[nodiscard]] double GetMaximumFrequencyHz() const noexcept;

    bool isNeutral_ = true;
};

} // namespace consolidator::dsp