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
    void UnregisterInstance(const AnalysisHandle& handle) noexcept;
    void SetView(AnalysisView view) noexcept;
    [[nodiscard]] std::optional<AnalysisView> GetView() const noexcept;
    [[nodiscard]] bool TryReadLatestSpectrum(SpectrumSnapshot& snapshot) noexcept;
    [[nodiscard]] bool TryReadLatestReferenceSpectrum(SpectrumSnapshot& snapshot) noexcept;
    [[nodiscard]] bool TryReadLatestDifferenceSpectrum(SpectrumSnapshot& snapshot) noexcept;
    [[nodiscard]] bool TryReadLatestCurve(EqualizerCurveSnapshot& snapshot) noexcept;

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
    SpectrumSnapshot latestMainSpectrum_;
    SpectrumSnapshot latestReferenceSpectrum_;
    bool hasMainSpectrum_ = false;
    bool hasReferenceSpectrum_ = false;
    std::uint64_t processedSpectrumViewRevision_ = 0;
    SpectrumAnalyzer spectrumAnalyzer_;
    SpectrumMapper spectrumMapper_;
    FrequencyResponseCalculator frequencyResponseCalculator_;
    std::jthread worker_;
};

} // namespace consolidator::analysis
