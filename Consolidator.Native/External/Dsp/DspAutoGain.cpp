#include "DspAutoGain.h"

#include <algorithm>
#include <cmath>

#include "DspMath.h"

namespace consolidator::max::dsp
{

void DspAutoGain::SetTuningBeforePrepare(
    const DspAutoGainTuning& tuning) noexcept
{
    tuning_ = tuning;
}

void DspAutoGain::Prepare(const DspProcessingContext& context) noexcept
{
    energyCoefficient_ = TimeCoefficient(tuning_.energySeconds, context.sampleRate);
    gainCoefficient_ = TimeCoefficient(tuning_.correctionSeconds, context.sampleRate);
}

void DspAutoGain::Reset() noexcept
{
    inputEnergy_ = 0.0;
    outputEnergy_ = 0.0;
    correctionDb_ = 0.0;
}

double DspAutoGain::Process(double inputEnergy, double outputEnergy, double activity) noexcept
{
    if (activity <= 0.0)
    {
        Reset();
        return 1.0;
    }
    if (inputEnergy > tuning_.minimumInputEnergy)
    {
        inputEnergy_ += energyCoefficient_ * (inputEnergy - inputEnergy_);
        outputEnergy_ += energyCoefficient_ * (outputEnergy - outputEnergy_);
        if (inputEnergy_ > 1.0e-15 && outputEnergy_ > 1.0e-15)
        {
            const auto desiredDb = std::clamp(
                tuning_.compensationRatio * 10.0 *
                    std::log10(inputEnergy_ / outputEnergy_),
                -tuning_.maximumCorrectionDb,
                tuning_.maximumCorrectionDb);
            correctionDb_ += gainCoefficient_ * (desiredDb - correctionDb_);
        }
    }
    return DbToGain(std::clamp(activity, 0.0, 1.0) * correctionDb_);
}

} // namespace consolidator::max::dsp
