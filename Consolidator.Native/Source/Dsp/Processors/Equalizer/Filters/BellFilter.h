#pragma once

#include "Core/Settings/DspDeviceSettings.h"
#include "Dsp/Processors/Equalizer/Filters/Filter.h"

namespace consolidator::dsp
{

// Implements a peaking/bell biquad filter.
class BellFilter final : public Filter
{
  public:
    BellFilter(FilterId filterId, double frequencyHz)
        : Filter(DeviceId::Equalizer, detail::ElementKind::EqFilter,
                 detail::ToIndex(filterId))
    {
        InitializeParameters(
            frequencyHz,
            core::settings::FilterDefaults::kDefaultQ,
            core::settings::FilterDefaults::kDefaultGainDb);
        RecalculateCoefficients();
    }

  protected:
    void RecalculateCoefficients() override
    {
        SetNormalizedCoefficients(BiquadDesigner::Calculate(
            BiquadType::Bell,
            runtimeState_.frequencyHz,
            runtimeState_.q,
            runtimeState_.gainDb,
            GetSampleRate()));
    }
};

} // namespace consolidator::dsp
