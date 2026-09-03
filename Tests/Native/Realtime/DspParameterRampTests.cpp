#include "TestSupport.h"

#include <cmath>

#include "Dsp/DspParameterRamp.h"

namespace consolidator::tests
{

bool RunDspParameterRampTests()
{
    consolidator::max::dsp::DspParameterRamp ramp{ 2.0 };
    ramp.Prepare({ 1000.0, 16 });
    ramp.SetTarget(12.0);

    auto succeeded = true;
    succeeded &= Expect(
        ramp.NextValue() == 2.0 && ramp.IsSmoothing(),
        "The parameter ramp did not begin from its current value.");

    ramp.Advance(4);
    succeeded &= Expect(
        std::abs(ramp.CurrentValue() - 7.0) < 1.0e-12,
        "The parameter ramp did not advance by the requested sample count.");

    ramp.SetTarget(12.0);
    ramp.Advance(5);
    succeeded &= Expect(
        ramp.CurrentValue() == 12.0 && !ramp.IsSmoothing(),
        "An unchanged target restarted parameter smoothing.");

    ramp.SetTarget(2.0);
    ramp.Advance(5);
    ramp.SetTarget(-3.0);
    ramp.Advance(10);
    succeeded &= Expect(
        ramp.CurrentValue() == -3.0 && ramp.TargetValue() == -3.0,
        "Parameter retargeting did not continue from the current value.");

    ramp.SetTarget(7.0);
    ramp.Reset();
    succeeded &= Expect(
        ramp.CurrentValue() == 7.0 && !ramp.IsSmoothing(),
        "Parameter reset did not synchronize current and target values.");
    return succeeded;
}

} // namespace consolidator::tests
