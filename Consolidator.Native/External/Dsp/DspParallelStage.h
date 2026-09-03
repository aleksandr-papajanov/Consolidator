#pragma once

#include <cstddef>
#include <vector>

#include "DspExecution.h"
#include "DspProcessor.h"

namespace consolidator::max::dsp
{

class DspParallelStage final
{
public:
    void Prepare(const DspProcessingContext& context);
    void Reset() noexcept;
    void Process(
        DspProcessingFrame& frame,
        DspProcessor& processor) noexcept;

private:
    std::vector<double> branchLeft_;
    std::vector<double> branchRight_;
};

} // namespace consolidator::max::dsp
