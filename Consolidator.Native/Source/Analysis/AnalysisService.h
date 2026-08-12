#pragma once

#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "Analysis/FrequencyResponse/FrequencyResponseCalculator.h"
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

    ~AnalysisService();

    AnalysisService(const AnalysisService&) = delete;
    AnalysisService& operator=(const AnalysisService&) = delete;

private:
    AnalysisService();

    void WorkerLoop(std::stop_token stopToken);
    [[nodiscard]] bool ProcessSpectrum(SpectrumStream& stream);
    [[nodiscard]] bool ProcessFrequencyResponse(
        FrequencyResponseStream& stream);
    [[nodiscard]] bool ProcessSlot(AnalysisSlot& slot);

    std::mutex slotsMutex_;
    std::vector<AnalysisHandle> slots_;
    SpectrumAnalyzer spectrumAnalyzer_;
    SpectrumMapper spectrumMapper_;
    FrequencyResponseCalculator frequencyResponseCalculator_;
    std::jthread worker_;
};

} // namespace consolidator::analysis
