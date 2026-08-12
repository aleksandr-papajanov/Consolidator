#include "Analysis/AnalysisService.h"

#include "Analysis/AnalysisSlot.h"

#include <algorithm>
#include <chrono>
#include <thread>

namespace consolidator::analysis
{

void AnalysisService::WorkerLoop(std::stop_token stopToken)
{
    while (!stopToken.stop_requested())
    {
        bool didWork = false;
        AnalysisHandle slot;
        std::optional<AnalysisView> view;
        std::uint64_t viewRevision = 0;
        {
            std::lock_guard lock{slotsMutex_};
            slot = FindViewedSlot();
            view = currentView_;
            viewRevision = viewRevision_;
        }

        if (slot && view)
        {
            SpectrumSnapshot mainSpectrum;
            SpectrumSnapshot referenceSpectrum;
            const auto mainProcessed = ProcessSpectrum(
                slot->MainSpectrum(), viewRevision, mainSpectrum);
            const auto referenceProcessed = ProcessSpectrum(
                slot->ReferenceSpectrum(), viewRevision, referenceSpectrum);
            if (mainProcessed)
            {
                latestMainSpectrum_ = mainSpectrum;
            }
            if (referenceProcessed)
            {
                latestReferenceSpectrum_ = referenceSpectrum;
            }
            didWork = mainProcessed || referenceProcessed || didWork;
            if (latestMainSpectrum_ && latestReferenceSpectrum_ &&
                latestMainSpectrum_->viewRevision == viewRevision &&
                latestReferenceSpectrum_->viewRevision == viewRevision &&
                (mainProcessed || referenceProcessed))
            {
                SpectrumSnapshot difference;
                difference.sampleRate = latestMainSpectrum_->sampleRate;
                difference.sourceRevision = std::max(
                    latestMainSpectrum_->sourceRevision,
                    latestReferenceSpectrum_->sourceRevision);
                difference.revision = nextResultRevision_++;
                difference.viewRevision = viewRevision;
                for (std::size_t index = 0;
                     index < difference.magnitudeDb.size();
                     ++index)
                {
                    difference.magnitudeDb[index] =
                        latestMainSpectrum_->magnitudeDb[index] -
                        latestReferenceSpectrum_->magnitudeDb[index];
                }
                differenceSpectrum_.Publish(difference);
                didWork = true;
            }
            didWork = ProcessFrequencyResponse(*slot, *view, viewRevision) || didWork;
        }

        if (!didWork)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds{2});
        }
        else
        {
            std::this_thread::yield();
        }
    }
}

} // namespace consolidator::analysis
