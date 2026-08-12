#pragma once

#include <atomic>

#include "Analysis/CurveInput.h"
#include "Analysis/FrequencyResponse/FrequencyResponseStream.h"
#include "Analysis/Spectrum/SpectrumStream.h"
#include "Analysis/TelemetryStream.h"
#include "Core/Domain/Ids/InstanceId.h"

namespace consolidator::analysis
{

// Publishes latest source data and results belonging to one instance.
class AnalysisSlot final
{
  public:
    explicit AnalysisSlot(core::InstanceId instanceId) noexcept
        : instanceId_(instanceId)
    {
    }

    [[nodiscard]] core::InstanceId GetInstanceId() const noexcept
    {
        return instanceId_;
    }

    void SetSpectrumEnabled(bool enabled) noexcept
    {
        spectrumEnabled_.store(enabled, std::memory_order_release);
    }

    [[nodiscard]] bool IsSpectrumEnabled() const noexcept
    {
        return spectrumEnabled_.load(std::memory_order_acquire);
    }

    void SetTelemetryEnabled(bool enabled) noexcept
    {
        telemetryEnabled_.store(enabled, std::memory_order_release);
    }

    [[nodiscard]] bool IsTelemetryEnabled() const noexcept
    {
        return telemetryEnabled_.load(std::memory_order_acquire);
    }

    [[nodiscard]] SpectrumStream& MainSpectrum() noexcept
    {
        return mainSpectrum_;
    }

    [[nodiscard]] SpectrumStream& ReferenceSpectrum() noexcept
    {
        return referenceSpectrum_;
    }

    [[nodiscard]] CurveState& Curves() noexcept
    {
        return curves_;
    }

    [[nodiscard]] FrequencyResponseStream& CurveOutput() noexcept
    {
        return curveOutput_;
    }

    [[nodiscard]] TelemetryStream& Telemetry() noexcept
    {
        return telemetry_;
    }

  private:
    core::InstanceId instanceId_;
    std::atomic_bool spectrumEnabled_{false};
    std::atomic_bool telemetryEnabled_{false};
    SpectrumStream mainSpectrum_;
    SpectrumStream referenceSpectrum_;
    CurveState curves_;
    FrequencyResponseStream curveOutput_;
    TelemetryStream telemetry_;
};

} // namespace consolidator::analysis
