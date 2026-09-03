#include "DspExecution.h"

namespace consolidator::max::dsp
{

DspExecutionPolicy BuildExecutionPolicy(
    const DspSnapshot& snapshot,
    const DspProcessingPlan& plan) noexcept
{
    return {
        snapshot.audible != 0,
        snapshot.instanceBypass != 0,
        snapshot.inputGainActive != 0 && snapshot.inputGainBypass == 0,
        snapshot.inputGainActive != 0 && snapshot.inputLeveler != 0,
        snapshot.saturatorActive != 0 && snapshot.saturatorBypass == 0,
        snapshot.saturatorSplit != 0,
        snapshot.compressorActive != 0 && snapshot.compressorBypass == 0,
        snapshot.compressorActive != 0 && snapshot.compressorParallel != 0,
        snapshot.equalizerActive != 0 && snapshot.equalizerBypass == 0,
        snapshot.polishBypass == 0,
        snapshot.outputGainActive != 0 && snapshot.outputGainBypass == 0,
        snapshot.outputGainActive != 0 && snapshot.outputLimiter != 0,
        snapshot.inputWidth != 100.0F,
        plan.channelMode
    };
}

} // namespace consolidator::max::dsp
