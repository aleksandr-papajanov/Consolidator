#pragma once

#include <memory>
#include <span>
#include <vector>

#include "Dsp/Parameters/RoutedParameterChange.h"
#include "Dsp/Processors/DspDevice.h"

namespace consolidator::dsp
{

class DspChain
{
public:
    DspChain() = default;
    ~DspChain() = default;

    DspChain(const DspChain&) = delete;
    DspChain& operator=(const DspChain&) = delete;

    void AddDevice(std::unique_ptr<DspDevice> device);
    void ApplyParameterChange(const RoutedParameterChange& change);

    void Process(const double* input,
                 double* interim,
                 double* output,
                 std::size_t frameCount,
                 std::size_t channelCount);

    [[nodiscard]] std::size_t GetDeviceCount() const noexcept;

    [[nodiscard]] DspDevice* GetDevice(std::size_t index) noexcept;
    [[nodiscard]] const DspDevice* GetDevice(std::size_t index) const noexcept;

private:
    std::vector<std::unique_ptr<DspDevice>> devices_;
};

} // namespace consolidator::dsp
