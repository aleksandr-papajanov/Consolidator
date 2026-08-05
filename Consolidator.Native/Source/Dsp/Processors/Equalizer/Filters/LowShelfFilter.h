#pragma once

#include "Core/Settings/DspDeviceSettings.h"
#include "Dsp/Processors/Equalizer/Filters/Filter.h"

namespace consolidator::dsp
{

struct LowShelfFilterState
{
    float frequency = static_cast<float>(
        core::settings::FilterDefaults::kDefaultFrequencyHz);

    float q = static_cast<float>(
        core::settings::FilterDefaults::kDefaultQ);

    float gainDb = static_cast<float>(
        core::settings::FilterDefaults::kDefaultGainDb);

    bool bypass = false;
};

class LowShelfFilter final : public Filter
{
public:
    LowShelfFilter(
        EqFilterId filterId,
        double frequencyHz);

    void SyncState();

    [[nodiscard]] const LowShelfFilterState& GetState() const noexcept
    {
        return state_;
    }

protected:
    void RecalculateCoefficients() override;

private:
    LowShelfFilterState state_{};
};

} // namespace consolidator::dsp