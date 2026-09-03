#pragma once

#include <cstddef>
#include <vector>

#include "DspExecution.h"

namespace consolidator::max::dsp
{

class DspParallelStage final
{
public:
    void Prepare(const DspProcessingContext& context);
    void Reset() noexcept;
    void SetEnabled(bool enabled) noexcept;
    void Process(DspProcessingFrame& frame) noexcept;

private:
    void ProcessBranch(
        double* left,
        double* right,
        const DspSidechainBuffer& sidechain,
        std::size_t frameCount) noexcept;

    std::vector<double> branchLeft_;
    std::vector<double> branchRight_;
    bool enabled_{};
};

} // namespace consolidator::max::dsp
