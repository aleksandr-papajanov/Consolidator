#include "TestSupport.h"

#include <array>
#include <cmath>

#include "Dsp/DspRuntime.h"

namespace consolidator::tests
{

bool RunDspRuntimeTests()
{
    consolidator::max::dsp::DspRuntime runtime;
    runtime.Prepare(1000.0, 5);

    consolidator::max::dsp::DspStateExchange exchange{};
    auto& initial = exchange.snapshots[1];
    initial.audible = 1;
    initial.inputGainActive = 1;
    initial.inputLevel = 6.0F;
    exchange.publishedIndex = 1;

    std::array<double, 5> left{};
    std::array<double, 5> right{};
    left.fill(1.0);
    right.fill(1.0);
    runtime.ProcessBlock(
        exchange,
        left.data(),
        right.data(),
        left.size());

    const auto targetGain = std::pow(10.0, 6.0 / 20.0);
    auto succeeded = true;
    succeeded &= Expect(
        left.front() == 1.0 &&
            left.back() > left.front() &&
            left.back() < targetGain &&
            exchange.consumerIndex == 1,
        "The audio-owned DSP runtime did not consume and ramp its initial snapshot.");

    auto& updated = exchange.snapshots[2];
    updated = initial;
    updated.inputLevel = 0.0F;
    exchange.publishedIndex = 2;
    left.fill(1.0);
    right.fill(1.0);
    runtime.ProcessBlock(
        exchange,
        left.data(),
        right.data(),
        left.size());

    succeeded &= Expect(
        left.front() > 1.0 &&
            left.back() > 1.0 &&
            left.back() < left.front() &&
            exchange.consumerIndex == 2,
        "A new DSP publication did not retarget smoothly from audio-owned runtime state.");

    auto& muted = exchange.snapshots[0];
    muted = updated;
    muted.audible = 0;
    exchange.publishedIndex = 0;
    left.fill(1.0);
    right.fill(1.0);
    runtime.ProcessBlock(
        exchange,
        left.data(),
        right.data(),
        left.size());

    succeeded &= Expect(
        left == std::array<double, 5>{} &&
            right == std::array<double, 5>{},
        "A DSP publication was not applied without a Managed output callback.");
    return succeeded;
}

} // namespace consolidator::tests
