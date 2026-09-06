#pragma once

#include <array>

#include "DspParameterRamp.h"
#include "DspProcessor.h"

namespace consolidator::max::dsp
{

class PolishAirProcessor final : public DspProcessor
{
public:
    void Prepare(const DspProcessingContext& context) noexcept override;
    void Reset() noexcept override;
    void SetParameters(const DspSnapshot& snapshot) noexcept override;
    void Process(DspProcessingFrame& frame) noexcept override;

private:
    struct Channel
    {
        std::array<double, 2> sourceHighPass{};
        std::array<double, 2> sourceLowPass{};
        std::array<double, 2> outputHighPass{};
        std::array<double, 1> shelf{};
        std::array<double, 1> topShelf{};
        double previousSample{};
    };

    DspParameterRamp amount_;
    std::array<Channel, 2> channels_{};
    double sourceHighPassCoefficient_{};
    double sourceLowPassCoefficient_{};
    double outputHighPassCoefficient_{};
    double shelfCoefficient_{};
    double topShelfCoefficient_{};
    double energyCoefficient_{};
    double fastEnergyCoefficient_{};
    double attenuationAttackCoefficient_{};
    double attenuationReleaseCoefficient_{};
    double bandEnergy_{};
    double mainEnergy_{};
    double fastBandEnergy_{};
    double energyWeight_{};
    double fastEnergyWeight_{};
    double attenuation_{ 1.0 };
};

} // namespace consolidator::max::dsp
