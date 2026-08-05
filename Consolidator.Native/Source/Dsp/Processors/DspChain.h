#pragma once

#include <memory>
#include <span>
#include <vector>

#include "Dsp/Parameters/ParameterBatch.h"
#include "Dsp/Parameters/ParameterChange.h"
#include "Dsp/Processors/IDspDevice.h"

namespace consolidator::dsp
{

class DspChain
{
public:
    DspChain() = default;
    ~DspChain() = default;

    DspChain(const DspChain&) = delete;
    DspChain& operator=(const DspChain&) = delete;

    void AddDevice(std::unique_ptr<IDspDevice> device);

    void ApplyParameterChange(const ParameterChange& change);
    void ApplyParameterBatch(const ParameterBatch& batch);

    void Process(const double* input,
                 double* interim,
                 double* output,
                 std::size_t frameCount,
                 std::size_t channelCount);

    [[nodiscard]] std::size_t GetDeviceCount() const noexcept;

    [[nodiscard]] IDspDevice* GetDevice(std::size_t index) noexcept;
    [[nodiscard]] const IDspDevice* GetDevice(std::size_t index) const noexcept;

private:
    void ApplyPendingChanges();
    void ApplyChangeToDevice(const ParameterChange& change);

    std::vector<std::unique_ptr<IDspDevice>> devices_;
    std::vector<ParameterChange> pendingChanges_;
};

} // namespace consolidator::dsp