#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <span>
#include <vector>

#include "Dsp/Processors/DspDevice.h"
#include "Core/Instance/Queues/RuntimeUpdateMailbox.h"

namespace consolidator::core
{
}

namespace consolidator::dsp
{

// Owns the ordered DSP devices and processes audio through the complete chain.
class DspChain final
{
public:
    DspChain() = default;
    ~DspChain() = default;

    DspChain(const DspChain&) = delete;
    DspChain& operator=(const DspChain&) = delete;

    void AddDevice(std::unique_ptr<DspDevice> device);
    // Prepares every device before the audio callback starts.
    void Prepare(
        double sampleRate,
        std::size_t channelCount);
    // Applies one coalesced mailbox batch before processing the next audio block.
    void ApplyRuntimeUpdates(const core::ParameterUpdateBatch& batch);
    // Applies active/listen runtime controls after parameters and before Process.
    void ApplyRuntimeControlUpdates(const core::RuntimeControlBatch& batch);
    // Resets runtime memory for devices matching the requested route.
    void Reset(const core::StatePath& target) noexcept;

    // Runs all active devices using preallocated intermediate buffers.
    void Process(const double* inputLeft,
                 const double* inputRight,
                 double* interimLeft,
                 double* interimRight,
                 double* outputLeft,
                 double* outputRight,
                 std::size_t frameCount);

    [[nodiscard]] std::size_t GetDeviceCount() const noexcept;

    [[nodiscard]] DspDevice* GetDevice(std::size_t index) noexcept;
    [[nodiscard]] const DspDevice* GetDevice(std::size_t index) const noexcept;

private:
    static constexpr std::size_t kMaximumDevices = 64;

    std::vector<std::unique_ptr<DspDevice>> devices_;
};

} // namespace consolidator::dsp
