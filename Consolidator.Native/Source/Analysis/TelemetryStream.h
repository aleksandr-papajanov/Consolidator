#pragma once

#include "Analysis/LatestSnapshot.h"
#include "Dsp/Telemetry/Telemetry.h"

namespace consolidator::analysis
{

// Publishes the newest DSP telemetry block without blocking the audio thread.
class TelemetryStream final
{
  public:
    using Snapshot = dsp::TelemetrySnapshot;

    void Publish(const Snapshot& snapshot) noexcept
    {
        latest_.Publish(snapshot);
    }

    [[nodiscard]] bool ReadLatest(
        Snapshot& snapshot,
        std::uint64_t lastRevision) const noexcept
    {
        return latest_.TryReadNewerThan(snapshot, lastRevision);
    }

  private:
    LatestSnapshot<Snapshot> latest_;
};

} // namespace consolidator::analysis
