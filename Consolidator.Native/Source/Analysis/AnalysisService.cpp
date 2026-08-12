#include "Analysis/AnalysisService.h"
#include "Analysis/AnalysisSlot.h"
#include "Analysis/FrequencyResponse/FrequencyResponseRequestBuilder.h"

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

AnalysisHandle AnalysisService::RegisterInstance(core::InstanceId instanceId)
{
    auto handle = std::make_shared<AnalysisSlot>(instanceId);
    std::lock_guard lock{slotsMutex_};
    if (currentView_ && instanceId == currentView_->instanceId)
    {
        handle->MainSpectrum().Reset();
        handle->ReferenceSpectrum().Reset();
        handle->SetSpectrumEnabled(true);
    }
    slots_.push_back(handle);
    return handle;
}

void AnalysisService::UnregisterInstance(const AnalysisHandle& handle)
{
    std::lock_guard lock{slotsMutex_};
    if (currentView_ && handle &&
        handle->GetInstanceId() == currentView_->instanceId)
    {
        currentView_.reset();
        ++viewRevision_;
    }
    std::erase(slots_, handle);
}

void AnalysisService::SetView(AnalysisView view)
{
    std::lock_guard lock{slotsMutex_};
    if (currentView_ && *currentView_ == view)
    {
        return;
    }

    for (const auto& slot : slots_)
    {
        slot->SetSpectrumEnabled(false);
    }

    currentView_ = view;
    ++viewRevision_;
    if (const auto slot = FindViewedSlot())
    {
        slot->MainSpectrum().Reset();
        slot->ReferenceSpectrum().Reset();
        slot->SetSpectrumEnabled(true);
    }
}

std::optional<AnalysisView> AnalysisService::GetView() const
{
    std::lock_guard lock{slotsMutex_};
    return currentView_;
}

AnalysisHandle AnalysisService::FindViewedSlot() const
{
    if (!currentView_)
    {
        return {};
    }

    for (const auto& slot : slots_)
    {
        if (slot->GetInstanceId() == currentView_->instanceId)
        {
            return slot;
        }
    }
    return {};
}

bool AnalysisService::TryReadLatestSpectrum(
    SpectrumSnapshot& snapshot,
    std::uint64_t lastRevision)
{
    std::lock_guard lock{slotsMutex_};
    const auto slot = FindViewedSlot();
    if (!slot || !slot->MainSpectrum().ReadLatestOutput(snapshot, lastRevision))
    {
        return false;
    }
    if (snapshot.viewRevision != viewRevision_)
    {
        return false;
    }
    return true;
}

bool AnalysisService::TryReadLatestReferenceSpectrum(
    SpectrumSnapshot& snapshot,
    std::uint64_t lastRevision)
{
    std::lock_guard lock{slotsMutex_};
    const auto slot = FindViewedSlot();
    if (!slot || !slot->ReferenceSpectrum().ReadLatestOutput(
            snapshot, lastRevision))
    {
        return false;
    }
    if (snapshot.viewRevision != viewRevision_)
    {
        return false;
    }
    return true;
}

bool AnalysisService::TryReadLatestDifferenceSpectrum(
    SpectrumSnapshot& snapshot,
    std::uint64_t lastRevision)
{
    std::lock_guard lock{slotsMutex_};
    if (!FindViewedSlot())
    {
        return false;
    }

    if (!differenceSpectrum_.TryReadNewerThan(snapshot, lastRevision))
        return false;
    if (snapshot.viewRevision != viewRevision_)
    {
        return false;
    }
    return true;
}

bool AnalysisService::TryReadLatestCurve(
    EqualizerCurveSnapshot& snapshot,
    std::uint64_t lastRevision)
{
    std::lock_guard lock{slotsMutex_};
    const auto slot = FindViewedSlot();
    if (!slot || !slot->CurveOutput().ReadLatestOutput(snapshot, lastRevision))
    {
        return false;
    }
    if (snapshot.viewRevision != viewRevision_)
    {
        return false;
    }
    return true;
}

bool AnalysisService::ProcessSpectrum(
    SpectrumStream& stream,
    std::uint64_t viewRevision,
    SpectrumSnapshot& output)
{
    AudioWindow input;
    if (!stream.TryConsumeInput(input))
    {
        return false;
    }

    RawSpectrum rawSpectrum;
    spectrumAnalyzer_.Calculate(input, rawSpectrum);

    spectrumMapper_.Calculate(rawSpectrum, output);
    output.revision = nextResultRevision_++;
    output.viewRevision = viewRevision;
    stream.PublishOutput(output);
    return true;
}

bool AnalysisService::ProcessFrequencyResponse(
    AnalysisSlot& slot,
    AnalysisView view,
    std::uint64_t viewRevision)
{
    const auto input = slot.Curves().Read();
    if (!slot.CurveOutput().NeedsProcessing(input.revision, viewRevision))
    {
        return false;
    }

    const auto request = FrequencyResponseRequestBuilder{}.Build(
        input, view);
    EqualizerCurveSnapshot output;
    for (std::size_t index = 0; index < request.filters.size(); ++index)
    {
        frequencyResponseCalculator_.Calculate(
            request.filters[index], output.filters[index]);
    }
    frequencyResponseCalculator_.Calculate(
        request.combined, output.combined);
    frequencyResponseCalculator_.Calculate(
        request.allBanksCombined, output.allBanksCombined);
    output.revision = nextResultRevision_++;
    output.viewRevision = viewRevision;
    for (auto& filter : output.filters)
    {
        filter.revision = output.revision;
        filter.viewRevision = viewRevision;
    }
    output.combined.revision = output.revision;
    output.combined.viewRevision = viewRevision;
    output.allBanksCombined.revision = output.revision;
    output.allBanksCombined.viewRevision = viewRevision;
    slot.CurveOutput().PublishOutput(output);
    slot.CurveOutput().MarkProcessed(input.revision, viewRevision);
    return true;
}

} // namespace consolidator::analysis
