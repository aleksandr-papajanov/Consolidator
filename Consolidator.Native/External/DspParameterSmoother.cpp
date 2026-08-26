#include "DspParameterSmoother.h"

#include <algorithm>
#include <cmath>

namespace consolidator::max
{

DspParameterSmoother::DspParameterSmoother() noexcept
{
    current_.gain = 1.0F;
    current_.saturatorMix = 1.0F;
    current_.saturatorDetectorAmount = 1.0F;
    current_.compressorThresholdDb = -24.0F;
    current_.compressorRatio = 4.0F;
    current_.compressorAttackMs = 10.0F;
    current_.compressorReleaseMs = 100.0F;
    current_.compressorMix = 1.0F;
    target_ = current_;
}

void DspParameterSmoother::Prepare(double sampleRate) noexcept
{
    if (std::isfinite(sampleRate) && sampleRate > 0.0)
    {
        sampleRate_ = sampleRate;
    }
}

void DspParameterSmoother::SetTarget(
    const DspSnapshot& target) noexcept
{
    const auto previous = current_;
    current_ = target;

    current_.gain = previous.gain;
    current_.saturatorDrive = previous.saturatorDrive;
    current_.saturatorOutputDb = previous.saturatorOutputDb;
    current_.saturatorMix = previous.saturatorMix;
    current_.saturatorDetectorAmount = previous.saturatorDetectorAmount;
    current_.compressorThresholdDb = previous.compressorThresholdDb;
    current_.compressorRatio = previous.compressorRatio;
    current_.compressorAttackMs = previous.compressorAttackMs;
    current_.compressorReleaseMs = previous.compressorReleaseMs;
    current_.compressorOutputDb = previous.compressorOutputDb;
    current_.compressorMix = previous.compressorMix;
    current_.outputGain = previous.outputGain;

    target_ = target;
    remainingSamples_ = static_cast<std::size_t>(
        std::max(1.0, std::round(sampleRate_ * kRampDurationSeconds)));
    const auto divisor = static_cast<float>(remainingSamples_);

    step_.gain = (target_.gain - current_.gain) / divisor;
    step_.saturatorDrive =
        (target_.saturatorDrive - current_.saturatorDrive) / divisor;
    step_.saturatorOutputDb =
        (target_.saturatorOutputDb - current_.saturatorOutputDb) / divisor;
    step_.saturatorMix =
        (target_.saturatorMix - current_.saturatorMix) / divisor;
    step_.saturatorDetectorAmount =
        (target_.saturatorDetectorAmount - current_.saturatorDetectorAmount) /
        divisor;
    step_.compressorThresholdDb =
        (target_.compressorThresholdDb - current_.compressorThresholdDb) /
        divisor;
    step_.compressorRatio =
        (target_.compressorRatio - current_.compressorRatio) / divisor;
    step_.compressorAttackMs =
        (target_.compressorAttackMs - current_.compressorAttackMs) / divisor;
    step_.compressorReleaseMs =
        (target_.compressorReleaseMs - current_.compressorReleaseMs) / divisor;
    step_.compressorOutputDb =
        (target_.compressorOutputDb - current_.compressorOutputDb) / divisor;
    step_.compressorMix =
        (target_.compressorMix - current_.compressorMix) / divisor;
    step_.outputGain =
        (target_.outputGain - current_.outputGain) / divisor;
}

const DspSnapshot& DspParameterSmoother::Advance() noexcept
{
    if (remainingSamples_ == 0)
    {
        return current_;
    }

    current_.gain += step_.gain;
    current_.saturatorDrive += step_.saturatorDrive;
    current_.saturatorOutputDb += step_.saturatorOutputDb;
    current_.saturatorMix += step_.saturatorMix;
    current_.saturatorDetectorAmount += step_.saturatorDetectorAmount;
    current_.compressorThresholdDb += step_.compressorThresholdDb;
    current_.compressorRatio += step_.compressorRatio;
    current_.compressorAttackMs += step_.compressorAttackMs;
    current_.compressorReleaseMs += step_.compressorReleaseMs;
    current_.compressorOutputDb += step_.compressorOutputDb;
    current_.compressorMix += step_.compressorMix;
    current_.outputGain += step_.outputGain;
    --remainingSamples_;

    if (remainingSamples_ == 0)
    {
        current_ = target_;
    }

    return current_;
}

const DspSnapshot& DspParameterSmoother::Current() const noexcept
{
    return current_;
}

} // namespace consolidator::max
