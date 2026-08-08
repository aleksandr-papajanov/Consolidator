#pragma once

#include "Core/Settings/DspDeviceSettings.h"
#include "Dsp/Processors/Equalizer/Filters/Filter.h"

namespace consolidator::dsp
{

class BellFilter final : public Filter
{
public:
    BellFilter(FilterId FilterId, double frequencyHz);

protected:
    void RecalculateCoefficients() override;
};

} // namespace consolidator::dsp