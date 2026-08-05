#pragma once

#include "Core/Settings/DspDeviceSettings.h"
#include "Dsp/Processors/Equalizer/Filters/Filter.h"

namespace consolidator::dsp
{

struct BellFilterState
{
    float frequency = static_cast<float>(
        core::settings::FilterDefaults::kDefaultFrequencyHz);

    float q = static_cast<float>(
        core::settings::FilterDefaults::kDefaultQ);

    float gainDb = static_cast<float>(
        core::settings::FilterDefaults::kDefaultGainDb);

    bool bypass = false;
};

class BellFilter final : public Filter
{
public:
    BellFilter(EqFilterId filterId, double frequencyHz);

    void SyncState();

    [[nodiscard]] const BellFilterState& GetState() const noexcept
    {
        return state_;
    }

protected:
    void RecalculateCoefficients() override;

private:
    BellFilterState state_{};
};

} // namespace consolidator::dsp