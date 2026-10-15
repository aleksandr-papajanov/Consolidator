#pragma once

#include "WetTransition.h"
#include "../Algorithms/Air/PolishAirProcessor.h"
#include "../Algorithms/Thick/PolishThickProcessor.h"

namespace consolidator::audio::devices
{

struct PolishParameters
{
    double thick{};
    double air{};
    bool bypass{};
};

class PolishDevice final
{
public:
    void Prepare(const ProcessingContext& context);
    void Reset() noexcept;
    void SetParameters(const PolishParameters& parameters) noexcept;
    void Process(AudioBuffer audio, ChannelMode mode) noexcept;

private:
    algorithms::PolishThickProcessor thick_;
    algorithms::PolishAirProcessor air_;
    WetTransition transition_;
};

} // namespace consolidator::audio::devices
