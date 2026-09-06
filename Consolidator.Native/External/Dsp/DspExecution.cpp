#include "DspExecution.h"

#include <cmath>

namespace consolidator::max::dsp
{

namespace
{

constexpr float kActivityEpsilon = 0.0001F;

[[nodiscard]] bool IsActive(float value) noexcept
{
    return std::abs(value) > kActivityEpsilon;
}

[[nodiscard]] bool HasActiveEqualizerBank(
    const DspSnapshot& snapshot) noexcept
{
    for (const auto active : snapshot.equalizerBanksActive)
    {
        if (active != 0)
        {
            return true;
        }
    }

    return false;
}

} // namespace

DspExecutionPolicy BuildExecutionPolicy(
    const DspSnapshot& snapshot,
    const DspProcessingPlan& plan) noexcept
{
    const auto saturatorActive = snapshot.saturatorActive != 0 &&
        snapshot.saturatorBypass == 0 &&
        (IsActive(snapshot.saturatorDrive) ||
            IsActive(snapshot.saturatorOutputDb));
    const auto compressorActive = snapshot.compressorActive != 0 &&
        snapshot.compressorBypass == 0 &&
        (IsActive(snapshot.compressorAttack) ||
            IsActive(snapshot.compressorSustain) ||
            IsActive(snapshot.compressorCompression) ||
            IsActive(snapshot.compressorOutputDb));
    const auto equalizerActive = snapshot.equalizerActive != 0 &&
        snapshot.equalizerBypass == 0 &&
        HasActiveEqualizerBank(snapshot);
    const auto polishActive = snapshot.polishBypass == 0 &&
        (IsActive(snapshot.polishThick) || IsActive(snapshot.polishAir));
    const auto outputBypassed = snapshot.outputGainBypass != 0;
    const auto limiterEnabled = !outputBypassed && snapshot.outputLimiter != 0;
    const auto outputActive = !outputBypassed &&
        (IsActive(snapshot.outputLevel) || limiterEnabled);

    return {
        snapshot.audible != 0,
        snapshot.instanceBypass != 0,
        snapshot.inputGainActive != 0,
        snapshot.inputGainBypass != 0,
        snapshot.inputGainActive != 0 &&
            snapshot.inputGainBypass == 0 &&
            snapshot.inputLeveler != 0,
        saturatorActive,
        saturatorActive && snapshot.saturatorSplit != 0,
        compressorActive,
        compressorActive && snapshot.compressorParallel != 0,
        equalizerActive,
        polishActive,
        outputActive,
        outputBypassed,
        limiterEnabled,
        snapshot.inputWidth != 1.0F,
        plan.channelMode
    };
}

} // namespace consolidator::max::dsp
