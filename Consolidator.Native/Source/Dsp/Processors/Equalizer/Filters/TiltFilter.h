#pragma once

#include "Core/Settings/DspDeviceSettings.h"
#include "Dsp/Processors/Equalizer/Filters/Filter.h"
#include "Dsp/Processors/Equalizer/Filters/HighShelfFilter.h"
#include "Dsp/Processors/Equalizer/Filters/LowShelfFilter.h"

namespace consolidator::dsp
{

class TiltFilter final : public Filter
{
public:
    TiltFilter(FilterId FilterId, double pivotHz);

    void Prepare(double sampleRate, std::size_t channelCount) override;

    void Reset() noexcept override;

    [[nodiscard]] double ProcessSample(double input, std::size_t channel) noexcept override;

protected:
    void RecalculateCoefficients() override;

    [[nodiscard]] bool CalculateIsNeutral() const noexcept override;

private:
    void ApplyInternalParameters();

    LowShelfFilter lowShelf_;
    HighShelfFilter highShelf_;
};

} // namespace consolidator::dsp
