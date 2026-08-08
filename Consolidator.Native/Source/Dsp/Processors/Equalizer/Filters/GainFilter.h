#pragma once

#include "Core/Settings/DspDeviceSettings.h"
#include "Dsp/Processors/Equalizer/Filters/Filter.h"

namespace consolidator::dsp
{

class GainFilter final : public Filter
{
public:
    GainFilter(FilterId FilterId);

protected:
    void RecalculateCoefficients() override;

};

} // namespace consolidator::dsp
