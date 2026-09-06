#pragma once

#include <array>

#include "DspParameterRamp.h"
#include "DspProcessor.h"

namespace consolidator::max::dsp
{

class PolishThickProcessor final : public DspProcessor
{
public:
    void Prepare(const DspProcessingContext& context) noexcept override;
    void Reset() noexcept override;
    void SetParameters(const DspSnapshot& snapshot) noexcept override;
    void Process(DspProcessingFrame& frame) noexcept override;

private:
    struct Channel
    {
        std::array<double, 1> rumble{};
        std::array<double, 2> source{};
        std::array<double, 2> highPass{};
        std::array<double, 2> lowPass{};
        double previousSample{};
    };

    DspParameterRamp amount_;
    std::array<Channel, 2> channels_{};
    double rumbleCoefficient_{};
    double sourceCoefficient_{};
    double highPassCoefficient_{};
    double lowPassCoefficient_{};
    double energyCoefficient_{};
    double energy_{};
    double energyWeight_{};
    double wetEnergy_{};
    double fastWetEnergy_{};
    double fastEnergyWeight_{};
    double fastEnergyCoefficient_{};
    double gainAttackCoefficient_{};
    double gainReleaseCoefficient_{};
    double harmonicGain_{ 1.0 };
    double sourcePeak_{};
    double peakAttackCoefficient_{};
    double peakReleaseCoefficient_{};
};

} // namespace consolidator::max::dsp
