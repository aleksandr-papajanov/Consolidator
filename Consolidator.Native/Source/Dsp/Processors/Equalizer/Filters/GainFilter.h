#pragma once

#include "Core/Settings/DspDeviceSettings.h"
#include "Dsp/Processors/Equalizer/Filters/Filter.h"

namespace consolidator::dsp
{

// Implements a frequency-independent gain filter.
class GainFilter final : public Filter
{
  public:
    explicit GainFilter(FilterId filterId)
        : Filter(DeviceId::Equalizer, detail::ElementKind::EqFilter,
                 detail::ToIndex(filterId))
    {
        RecalculateCoefficients();
    }

  protected:
    void RecalculateCoefficients() override
    {
        SetNormalizedCoefficients(BiquadDesigner::Calculate(
            BiquadType::Gain, 0.0, 1.0, runtimeState_.gainDb, 1.0));
    }
};

} // namespace consolidator::dsp
