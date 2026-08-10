#pragma once

#include "Core/Settings/DspDeviceSettings.h"
#include "Dsp/Processors/Equalizer/Filters/Filter.h"

namespace consolidator::dsp
{

// Implements a low-frequency shelving biquad filter.
class LowShelfFilter final : public Filter
{
public:
    LowShelfFilter(FilterId FilterId, double frequencyHz);

protected:
    void RecalculateCoefficients() override;
};

} // namespace consolidator::dsp
