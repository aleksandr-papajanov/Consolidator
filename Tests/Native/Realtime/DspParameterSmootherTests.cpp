#include "TestSupport.h"

#include <cmath>

#include "DspParameterSmoother.h"

namespace consolidator::tests
{

namespace
{

bool IsClose(float actual, float expected)
{
    return std::abs(actual - expected) < 0.0001F;
}

}

bool RunDspParameterSmootherTests()
{
    consolidator::max::DspParameterSmoother smoother;
    smoother.Prepare(1000.0);

    auto target = smoother.Current();
    target.gain = 2.0F;
    target.saturatorDrive = 3.0F;
    target.saturatorOutputDb = 4.0F;
    target.saturatorMix = 0.5F;
    target.saturatorDetectorAmount = 0.25F;
    target.compressorThresholdDb = -12.0F;
    target.compressorRatio = 8.0F;
    target.compressorAttackMs = 20.0F;
    target.compressorReleaseMs = 200.0F;
    target.compressorOutputDb = 6.0F;
    target.compressorMix = 0.75F;
    target.outputGain = 1.5F;
    target.saturatorBypass = 1;

    smoother.SetTarget(target);

    auto succeeded = true;
    succeeded &= Expect(
        smoother.Current().saturatorBypass == 1 &&
            IsClose(smoother.Current().gain, 1.0F),
        "Discrete DSP parameters were ramped or a continuous parameter jumped.");

    for (auto sample = 0; sample < 10; ++sample)
    {
        smoother.Advance();
    }

    const auto& completed = smoother.Current();
    succeeded &= Expect(
        IsClose(completed.gain, target.gain) &&
            IsClose(completed.saturatorDrive, target.saturatorDrive) &&
            IsClose(completed.saturatorOutputDb, target.saturatorOutputDb) &&
            IsClose(completed.saturatorMix, target.saturatorMix) &&
            IsClose(
                completed.saturatorDetectorAmount,
                target.saturatorDetectorAmount) &&
            IsClose(
                completed.compressorThresholdDb,
                target.compressorThresholdDb) &&
            IsClose(completed.compressorRatio, target.compressorRatio) &&
            IsClose(
                completed.compressorAttackMs,
                target.compressorAttackMs) &&
            IsClose(
                completed.compressorReleaseMs,
                target.compressorReleaseMs) &&
            IsClose(
                completed.compressorOutputDb,
                target.compressorOutputDb) &&
            IsClose(completed.compressorMix, target.compressorMix) &&
            IsClose(completed.outputGain, target.outputGain),
        "The timed DSP ramp did not reach every continuous target.");

    target.gain = 4.0F;
    smoother.SetTarget(target);
    for (auto sample = 0; sample < 5; ++sample)
    {
        smoother.Advance();
    }

    const auto midpoint = smoother.Current().gain;
    target.gain = 1.0F;
    smoother.SetTarget(target);
    smoother.Advance();

    succeeded &= Expect(
        smoother.Current().gain < midpoint &&
            smoother.Current().gain > target.gain,
        "Retargeting did not continue smoothly from the current DSP value.");

    for (auto sample = 1; sample < 10; ++sample)
    {
        smoother.Advance();
    }

    succeeded &= Expect(
        IsClose(smoother.Current().gain, target.gain),
        "A retargeted DSP ramp did not reach its exact target.");
    return succeeded;
}

} // namespace consolidator::tests
