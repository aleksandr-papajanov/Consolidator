#pragma once

#include "DspProcessingContext.h"

namespace consolidator::max::dsp
{

class DspAutoGain final
{
public:
    void Prepare(const DspProcessingContext& context) noexcept;
    void Reset() noexcept;
    [[nodiscard]] double Process(double inputEnergy, double outputEnergy, double activity) noexcept;

private:
    double inputEnergy_{};
    double outputEnergy_{};
    double correctionDb_{};
    double energyCoefficient_{};
    double gainCoefficient_{};
};

} // namespace consolidator::max::dsp
