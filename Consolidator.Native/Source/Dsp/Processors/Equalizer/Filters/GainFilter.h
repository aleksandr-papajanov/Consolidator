#pragma once

#include "Core/Settings/DspDeviceSettings.h"
#include "Dsp/Processors/Equalizer/Filters/Filter.h"

namespace consolidator::dsp
{

struct GainFilterState
{
    float gainDb = static_cast<float>(core::settings::FilterDefaults::kDefaultGainDb);
    bool bypass = false;
};

class GainFilter final : public Filter
{
public:
    GainFilter(EqFilterId filterId);

    void SyncState();

    [[nodiscard]] const GainFilterState& GetState() const noexcept
    {
        return state_;
    }

protected:
    void RecalculateCoefficients() override;

private:
    GainFilterState state_{};
};

} // namespace consolidator::dsp