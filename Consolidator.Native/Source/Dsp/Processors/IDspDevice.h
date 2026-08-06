#pragma once

#include <cstddef>

#include "Dsp/Parameters/ParameterAddress.h"
#include "Dsp/Parameters/ParameterChange.h"

namespace consolidator::dsp
{

class IDspDevice
{
public:
    virtual ~IDspDevice() = default;

    virtual void Process(const double* input,
                         double* output,
                         std::size_t frameCount,
                         std::size_t channelCount) = 0;

    virtual void ApplyParameterChange(const ParameterChange& change) = 0;

    [[nodiscard]] virtual DeviceId GetDeviceId() const noexcept = 0;

    // Returns the element kind and index for routing.
    // For filters: EqFilter with index 0..6.
    // For detector sidechains: SaturatorDetectorFilter or CompressorDetectorFilter.
    // For plain devices (Gain, Saturator, Compressor): Device with index 0.
    [[nodiscard]] virtual detail::ElementKind GetElementKind() const noexcept = 0;
    [[nodiscard]] virtual std::uint8_t GetElementIndex() const noexcept = 0;

    [[nodiscard]] virtual bool IsNeutral() const noexcept = 0;
};

} // namespace consolidator::dsp