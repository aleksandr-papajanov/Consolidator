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
    target.inputLevel = 2.0F;
    target.saturatorDrive = 3.0F;
    target.saturatorOutputDb = 4.0F;
    target.saturatorCurve = 0.5F;
    target.compressorAttack = 0.25F;
    target.compressorSustain = 0.75F;
    target.compressorCompression = 0.8F;
    target.compressorCharacter = 2;
    target.compressorParallel = 1;
    target.compressorOutputDb = 6.0F;
    target.polishThick = 0.75F;
    target.outputLevel = 1.5F;
    target.saturatorBypass = 1;

    smoother.SetTarget(target);

    auto succeeded = true;
    succeeded &= Expect(
        smoother.Current().saturatorBypass == 1 &&
            IsClose(smoother.Current().inputLevel, 0.0F),
        "Discrete DSP parameters were ramped or a continuous parameter jumped.");

    for (auto sample = 0; sample < 10; ++sample)
    {
        smoother.Advance();
    }

    const auto& completed = smoother.Current();
    succeeded &= Expect(
        IsClose(completed.inputLevel, target.inputLevel) &&
            IsClose(completed.saturatorDrive, target.saturatorDrive) &&
            IsClose(completed.saturatorOutputDb, target.saturatorOutputDb) &&
            IsClose(completed.saturatorCurve, target.saturatorCurve) &&
            IsClose(completed.compressorAttack, target.compressorAttack) &&
            IsClose(completed.compressorSustain, target.compressorSustain) &&
            IsClose(completed.compressorCompression, target.compressorCompression) &&
            completed.compressorCharacter == target.compressorCharacter &&
            completed.compressorParallel == target.compressorParallel &&
            IsClose(
                completed.compressorOutputDb,
                target.compressorOutputDb) &&
            IsClose(completed.polishThick, target.polishThick) &&
            IsClose(completed.outputLevel, target.outputLevel),
        "The timed DSP ramp did not reach every continuous target.");

    target.inputLevel = 4.0F;
    smoother.SetTarget(target);
    for (auto sample = 0; sample < 5; ++sample)
    {
        smoother.Advance();
    }

    const auto midpoint = smoother.Current().inputLevel;
    target.inputLevel = 1.0F;
    smoother.SetTarget(target);
    smoother.Advance();

    succeeded &= Expect(
        smoother.Current().inputLevel < midpoint &&
            smoother.Current().inputLevel > target.inputLevel,
        "Retargeting did not continue smoothly from the current DSP value.");

    for (auto sample = 1; sample < 10; ++sample)
    {
        smoother.Advance();
    }

    succeeded &= Expect(
        IsClose(smoother.Current().inputLevel, target.inputLevel),
        "A retargeted DSP ramp did not reach its exact target.");
    return succeeded;
}

} // namespace consolidator::tests
