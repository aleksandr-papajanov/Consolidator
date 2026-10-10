#pragma once

#include "DspProcessingContext.h"

namespace consolidator::max::dsp
{

struct DspAutoGainTuning
{
    bool enabled{ true };
    double energySeconds{ 1.0 };
    double correctionSeconds{ 2.0 };
    double compensationRatio{ 0.5 };
    double maximumCorrectionDb{ 3.0 };
    double minimumInputEnergy{ 1.0e-10 };
};

class DspAutoGain final
{
public:
    void SetTuningBeforePrepare(const DspAutoGainTuning& tuning) noexcept;
    void Prepare(const DspProcessingContext& context) noexcept;
    void Reset() noexcept;
    [[nodiscard]] double Process(double inputEnergy, double outputEnergy, double activity) noexcept;

private:
    DspAutoGainTuning tuning_;
    double inputEnergy_{};
    double outputEnergy_{};
    double correctionDb_{};
    double energyCoefficient_{};
    double gainCoefficient_{};
};

} // namespace consolidator::max::dsp
