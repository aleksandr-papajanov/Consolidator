#pragma once

#include "DspExecution.h"
#include "SharedDspState.h"

namespace consolidator::max::dsp
{

class DspProcessor
{
public:
    virtual ~DspProcessor() = default;

    virtual void Prepare(const DspProcessingContext& context) = 0;
    virtual void Reset() noexcept = 0;
    virtual void SetParameters(const DspSnapshot& snapshot) noexcept = 0;
    virtual void Process(DspProcessingFrame& frame) noexcept = 0;
};

} // namespace consolidator::max::dsp
