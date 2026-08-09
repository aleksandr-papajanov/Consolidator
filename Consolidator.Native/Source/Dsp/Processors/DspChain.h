#pragma once

#include <memory>
#include <optional>
#include <span>
#include <vector>

#include "Dsp/Processors/DspDevice.h"

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
    void ReadState(const core::StatePath& path, core::StateResponseEntries& snapshot) const;
    core::StateWriteStatus WriteState(const core::StateEntry& entry, core::StateResponseEntries& applied);

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
