#pragma once

#include <array>
#include <cstddef>

#include "Core/Settings/DspDeviceSettings.h"
#include "Dsp/Parameters/ParameterAddress.h"
#include "Dsp/Processors/IDspDevice.h"
#include "Dsp/Processors/Saturator/EnvelopeDetector.h"

namespace consolidator::dsp
{

struct SaturatorState
{
    float drive =
        static_cast<float>(
            core::settings::SaturatorDefaults::kDefaultDrive);

    float outputDb =
        static_cast<float>(
            core::settings::SaturatorDefaults::kDefaultOutputDb);

    float mix =
        static_cast<float>(
            core::settings::SaturatorDefaults::kDefaultMix);

    float detectorAmount = 1.0f;
    bool bypass = false;
};

struct SaturatorRuntime
{
    double driveLinear = 1.0;
    double outputGainLinear = 1.0;

    double wetMix = 1.0;
    double dryMix = 0.0;

    double detectorAmount = 1.0;
};

class Saturator final : public IDspDevice
{
public:
    Saturator();

    void Prepare(
        double sampleRate,
        std::size_t channelCount);

    void Reset() noexcept;

    void Process(
        const double* input,
        double* output,
        std::size_t frameCount,
        std::size_t channelCount) override;

    void ApplyParameterChange(
        const ParameterChange& change) override;

    [[nodiscard]] DeviceId GetDeviceId() const noexcept override
    {
        return DeviceId::Saturator;
    }

    [[nodiscard]] detail::ElementKind GetElementKind() const noexcept override
    {
        return detail::ElementKind::Device;
    }

    [[nodiscard]] std::uint8_t GetElementIndex() const noexcept override
    {
        return 0;
    }

    [[nodiscard]] const SaturatorState& GetState() const noexcept
    {
        return state_;
    }

private:
    static constexpr std::size_t kMaximumChannelCount = 2;

    static constexpr float kMinimumDrive = 0.1f;
    static constexpr float kMaximumDrive = 100.0f;

    static constexpr float kMinimumMix = 0.0f;
    static constexpr float kMaximumMix = 1.0f;

    static constexpr float kMinimumDetectorAmount = 0.0f;
    static constexpr float kMaximumDetectorAmount = 8.0f;

    static constexpr double kMaximumDriveModulation = 16.0;

    void RecalculateRuntime();

    void ApplyDeviceParameter(const ParameterChange& change);

    void ApplyDetectorParameter(const ParameterChange& change);

    [[nodiscard]] double ProcessSample(double input, EnvelopeDetector& detector) const noexcept;

    [[nodiscard]] double CalculateDriveModulation(double envelope) const noexcept;

    [[nodiscard]] double ApplyWaveshaper(double input, double drive) const noexcept;

    void SetDrive(float drive);
    void SetOutputDb(float outputDb);
    void SetMix(float mix);
    void SetDetectorAmount(float amount);
    void SetBypass(bool bypass) noexcept;

    SaturatorState state_;
    SaturatorRuntime runtime_;

    std::size_t activeChannelCount_ = kMaximumChannelCount;

    std::array<EnvelopeDetector, kMaximumChannelCount> detectors_{
        EnvelopeDetector{
            SaturatorDetectorFilterId::Filter1,
            SaturatorDetectorFilterId::Filter2
        },
        EnvelopeDetector{
            SaturatorDetectorFilterId::Filter1,
            SaturatorDetectorFilterId::Filter2
        }
    };
};

} // namespace consolidator::dsp