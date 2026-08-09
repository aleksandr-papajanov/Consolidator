#pragma once

#include <memory>
#include <optional>
#include <span>
#include <vector>

#include "Core/Parameters/RoutedParameterChange.h"
#include "Core/State/IStateSource.h"
#include "Dsp/Processors/DspDevice.h"

namespace consolidator::core
{
}

namespace consolidator::dsp
{

class DspChain final : public core::IStateSource
{
public:
    DspChain() = default;
    ~DspChain() = default;

    DspChain(const DspChain&) = delete;
    DspChain& operator=(const DspChain&) = delete;

    void AddDevice(std::unique_ptr<DspDevice> device);
    void ApplyParameterChange(const RoutedParameterChange& change);
    void AppendState(const core::StatePath& path, core::StateSnapshot& snapshot) const override;

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
