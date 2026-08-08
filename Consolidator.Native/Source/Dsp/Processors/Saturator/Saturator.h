#pragma once

#include <array>
#include <cstddef>

#include "Core/Settings/DspDeviceSettings.h"
#include "Dsp/Parameters/DspParameter.h"
#include "Dsp/Processors/DspDevice.h"
#include "Dsp/Processors/Saturator/DetectorEnvelopeFollower.h"

namespace consolidator::dsp
{

struct SaturatorState
{
    DspParameter<float> drive{
        ParameterId::Drive,
        static_cast<float>(core::settings::SaturatorDefaults::kDefaultDrive),
        static_cast<float>(core::settings::SaturatorDefaults::kMinDrive),
        static_cast<float>(core::settings::SaturatorDefaults::kMaxDrive)};

    DspParameter<float> outputDb{
        ParameterId::Gain,
        static_cast<float>(core::settings::SaturatorDefaults::kDefaultOutputDb),
        static_cast<float>(core::settings::SaturatorDefaults::kMinOutputDb),
        static_cast<float>(core::settings::SaturatorDefaults::kMaxOutputDb)};

    DspParameter<float> mix{
        ParameterId::Mix,
        static_cast<float>(core::settings::SaturatorDefaults::kDefaultMix),
        static_cast<float>(core::settings::SaturatorDefaults::kMinMix),
        static_cast<float>(core::settings::SaturatorDefaults::kMaxMix)};

    DspParameter<float> detectorAmount{
        ParameterId::Type,
        1.0f,
        0.0f,
        8.0f};
        
    DspParameter<bool> bypass{
        ParameterId::Bypass,
        false};
};

struct SaturatorRuntimeState
{
    double driveLinear = 1.0;
    double outputGainLinear = 1.0;

    double wetMix = 1.0;
    double dryMix = 0.0;

    double detectorAmount = 1.0;
    bool isNeutral = true;
};

class Saturator final : public DspDevice
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

    [[nodiscard]] const SaturatorState& GetState() const noexcept
    {
        return state_;
    }

    [[nodiscard]] const SaturatorRuntimeState& GetRuntimeState() const noexcept
    {
        return runtimeState_;
    }

    bool ApplyParameter(
        const ParameterRoute& route,
        const ParameterValue& value,
        std::size_t depth) override;

    [[nodiscard]] bool IsNeutral() const noexcept override
    {
        return runtimeState_.isNeutral;
    }

private:
    static constexpr std::size_t kMaximumChannelCount = 2;

    bool ApplyStateParameter(
        const ParameterRoute& route,
        const ParameterValue& value) override;
    void RecalculateRuntime() override;


    [[nodiscard]] double ProcessSample(
        double input,
        DetectorEnvelopeFollower& detector) const noexcept;

    [[nodiscard]] double CalculateDriveModulation(
        double envelope) const noexcept;

    [[nodiscard]] double ApplyWaveshaper(
        double input,
        double drive) const noexcept;

    void SetDrive(float drive);
    void SetOutputDb(float outputDb);
    void SetMix(float mix);
    void SetDetectorAmount(float amount);
    void SetBypass(bool bypass) noexcept;

    SaturatorState state_;
    SaturatorRuntimeState runtimeState_;

    std::size_t activeChannelCount_ = kMaximumChannelCount;

    std::array<DetectorEnvelopeFollower, kMaximumChannelCount> detectors_{
        DetectorEnvelopeFollower{
            SaturatorDetectorFilterId::Filter1,
            SaturatorDetectorFilterId::Filter2
        },
        DetectorEnvelopeFollower{
            SaturatorDetectorFilterId::Filter1,
            SaturatorDetectorFilterId::Filter2
        }
    };
};

} // namespace consolidator::dsp
