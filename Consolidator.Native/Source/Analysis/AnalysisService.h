#pragma once

#include <memory>
#include <mutex>
#include <cstdint>
#include <optional>
#include <thread>
#include <vector>

#include "Analysis/AnalysisView.h"
#include "Analysis/CurveInput.h"
#include "Analysis/FrequencyResponse/FrequencyResponseCalculator.h"
#include "Analysis/LatestValue.h"
#include "Analysis/Spectrum/SpectrumMapper.h"
#include "Analysis/Spectrum/SpectrumAnalyzer.h"
#include "Dsp/Telemetry/Telemetry.h"
#include "Core/Domain/Ids/InstanceId.h"

namespace consolidator::analysis
{

class AnalysisSlot;
class SpectrumStream;
class FrequencyResponseStream;
using AnalysisHandle = std::shared_ptr<AnalysisSlot>;

// Owns the process-wide analysis worker and one latest-value slot per instance.
class AnalysisService final
{
  public:
    static AnalysisService& Get();

    AnalysisHandle RegisterInstance(core::InstanceId instanceId);
    void UnregisterInstance(const AnalysisHandle& handle);
    void SetView(AnalysisView view);
    [[nodiscard]] std::optional<AnalysisView> GetView() const;
    [[nodiscard]] bool TryReadLatestSpectrum(
        SpectrumSnapshot& snapshot,
        std::uint64_t lastRevision);
    [[nodiscard]] bool TryReadLatestReferenceSpectrum(
        SpectrumSnapshot& snapshot,
        std::uint64_t lastRevision);
    [[nodiscard]] bool TryReadLatestDifferenceSpectrum(
        SpectrumSnapshot& snapshot,
        std::uint64_t lastRevision);
    [[nodiscard]] bool TryReadLatestCurve(
        EqualizerCurveSnapshot& snapshot,
        std::uint64_t lastRevision);
    [[nodiscard]] bool TryReadLatestTelemetry(
        dsp::TelemetrySnapshot& snapshot,
        std::uint64_t lastRevision,
        std::uint64_t lastViewRevision);

    ~AnalysisService();

    AnalysisService(const AnalysisService&) = delete;
    AnalysisService& operator=(const AnalysisService&) = delete;

  private:
    AnalysisService();

    void WorkerLoop(std::stop_token stopToken);
    [[nodiscard]] bool ProcessSpectrum(
        SpectrumStream& stream,
        std::uint64_t viewRevision,
        SpectrumSnapshot& output);
    [[nodiscard]] bool ProcessFrequencyResponse(
        AnalysisSlot& slot,
        AnalysisView view,
        std::uint64_t viewRevision);
    [[nodiscard]] AnalysisHandle FindViewedSlot() const;

    mutable std::mutex slotsMutex_;
    std::vector<AnalysisHandle> slots_;
    std::optional<AnalysisView> currentView_;
    std::uint64_t viewRevision_ = 0;
    std::uint64_t nextResultRevision_ = 1;
    LatestValue<SpectrumSnapshot> differenceSpectrum_;
    std::optional<SpectrumSnapshot> latestMainSpectrum_;
    std::optional<SpectrumSnapshot> latestReferenceSpectrum_;
    SpectrumAnalyzer spectrumAnalyzer_;
    SpectrumMapper spectrumMapper_;
    FrequencyResponseCalculator frequencyResponseCalculator_;
    std::jthread worker_;
};

} // namespace consolidator::analysis
