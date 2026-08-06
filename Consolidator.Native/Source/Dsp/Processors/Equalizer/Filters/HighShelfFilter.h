#pragma once

#include "Core/Settings/DspDeviceSettings.h"
#include "Dsp/Processors/Equalizer/Filters/Filter.h"

namespace consolidator::dsp
{

struct HighShelfFilterState
{
    float frequency = static_cast<float>(core::settings::FilterDefaults::kDefaultFrequencyHz);
    float q = static_cast<float>(core::settings::FilterDefaults::kDefaultQ);
    float gainDb = static_cast<float>(core::settings::FilterDefaults::kDefaultGainDb);
    bool bypass = false;
};

class HighShelfFilter final : public Filter
{
public:
    HighShelfFilter(EqFilterId filterId, double frequencyHz);

    void SyncState();

    [[nodiscard]] const HighShelfFilterState& GetState() const noexcept
    {
        return state_;
    }

protected:
    void RecalculateCoefficients() override;

private:
    HighShelfFilterState state_{};
};

} // namespace consolidator::dsp