#pragma once

#include "Core/Settings/DspDeviceSettings.h"
#include "Dsp/Processors/Equalizer/Filters/Filter.h"

namespace consolidator::dsp
{

// Implements a high-frequency shelving biquad filter.
class HighShelfFilter final : public Filter
{
  public:
    HighShelfFilter(FilterId filterId, double frequencyHz)
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
            BiquadType::HighShelf,
            runtimeState_.frequencyHz,
            runtimeState_.q,
            runtimeState_.gainDb,
            GetSampleRate()));
    }
};

} // namespace consolidator::dsp
