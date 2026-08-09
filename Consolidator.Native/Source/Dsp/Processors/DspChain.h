#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <span>
#include <vector>

#include "Dsp/Processors/DspDevice.h"
#include "Core/Instance/Queues/DspUpdateMailbox.h"

namespace consolidator::core
{
}

namespace consolidator::dsp
{

class DspChain final
{
public:
    DspChain() = default;
    ~DspChain() = default;

    DspChain(const DspChain&) = delete;
    DspChain& operator=(const DspChain&) = delete;

    void AddDevice(std::unique_ptr<DspDevice> device);
    void ApplyRuntimeUpdates(const core::DspStateBatch& batch);

    void Process(const double* input,
                 double* interim,
                 double* output,
                 std::size_t frameCount,
                 std::size_t channelCount);

    [[nodiscard]] std::size_t GetDeviceCount() const noexcept;

    [[nodiscard]] DspDevice* GetDevice(std::size_t index) noexcept;
    [[nodiscard]] const DspDevice* GetDevice(std::size_t index) const noexcept;

private:
    static constexpr std::size_t kMaximumDevices = 64;

    std::vector<std::unique_ptr<DspDevice>> devices_;
};

} // namespace consolidator::dsp
