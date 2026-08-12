#include "Analysis/AnalysisService.h"
#include "Analysis/AnalysisSlot.h"

namespace consolidator::analysis
{

AnalysisService& AnalysisService::Get()
{
    static AnalysisService service;
    return service;
}

AnalysisService::AnalysisService()
    : worker_([this](std::stop_token stopToken)
      {
          WorkerLoop(stopToken);
      })
{
}

AnalysisService::~AnalysisService()
{
    worker_.request_stop();
    if (worker_.joinable())
    {
        worker_.join();
    }
}

AnalysisHandle AnalysisService::RegisterInstance(core::InstanceId)
{
    auto handle = std::make_shared<AnalysisSlot>();
    {
        std::lock_guard lock{slotsMutex_};
        slots_.push_back(handle);
    }
    return handle;
}

void AnalysisService::UnregisterInstance(const AnalysisHandle& handle) noexcept
{
    std::lock_guard lock{slotsMutex_};
    std::erase(slots_, handle);
}

bool AnalysisService::ProcessSpectrum(SpectrumStream& stream)
{
    AudioWindow input;
    if (!stream.TryConsumeInput(input))
    {
        return false;
    }

    RawSpectrum rawSpectrum;
    spectrumAnalyzer_.Calculate(input, rawSpectrum);

    AudioWindow newerInput;
    if (stream.TryConsumeInput(newerInput))
    {
        spectrumAnalyzer_.Calculate(newerInput, rawSpectrum);
    }

    SpectrumSnapshot output;
    spectrumMapper_.Calculate(rawSpectrum, output);
    stream.PublishOutput(output);
    return true;
}

bool AnalysisService::ProcessFrequencyResponse(
    FrequencyResponseStream& stream)
{
    FrequencyResponseRequest input;
    if (!stream.TryConsumeRequest(input))
    {
        return false;
    }

    FrequencyResponseSnapshot output;
    frequencyResponseCalculator_.Calculate(input, output);

    FrequencyResponseRequest newerInput;
    if (stream.TryConsumeRequest(newerInput))
    {
        frequencyResponseCalculator_.Calculate(newerInput, output);
    }
    stream.PublishOutput(output);
    return true;
}

bool AnalysisService::ProcessSlot(AnalysisSlot& slot)
{
    const auto mainProcessed = ProcessSpectrum(slot.MainSpectrum());
    const auto referenceProcessed = ProcessSpectrum(slot.ReferenceSpectrum());
    const auto equalizerProcessed = ProcessFrequencyResponse(
        slot.EqualizerResponse());
    return mainProcessed || referenceProcessed || equalizerProcessed;
}

} // namespace consolidator::analysis
