#pragma once

#include <cstddef>
#include <vector>

#include "DspExecution.h"
#include "DspProcessor.h"
#include "DspParameterRamp.h"

namespace consolidator::max::dsp
{

class DspParallelStage final
{
public:
    void Prepare(const DspProcessingContext& context);
    void Reset() noexcept;
    void SetWetTarget(double wet) noexcept;
    void Process(
        DspProcessingFrame& frame,
        DspProcessor& processor) noexcept;

private:
    std::vector<double> branchLeft_;
    std::vector<double> branchRight_;
    DspParameterRamp wet_;
    bool wasProcessing_{};
};

} // namespace consolidator::max::dsp
