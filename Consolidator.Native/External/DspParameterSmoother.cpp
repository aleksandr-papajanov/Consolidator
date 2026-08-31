#include "DspParameterSmoother.h"

#include <algorithm>
#include <cmath>

namespace consolidator::max
{

DspParameterSmoother::DspParameterSmoother() noexcept
{
    current_.inputTarget = -18.0F;
    current_.inputWidth = 100.0F;
    current_.saturatorCurve = 0.5F;
    current_.compressorAttack = 0.5F;
    current_.compressorSustain = 0.5F;
    current_.compressorCompression = 0.5F;
    current_.outputTarget = -1.0F;
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

    current_.inputLevel = previous.inputLevel;
    current_.inputTarget = previous.inputTarget;
    current_.inputWidth = previous.inputWidth;
    current_.saturatorDrive = previous.saturatorDrive;
    current_.saturatorOutputDb = previous.saturatorOutputDb;
    current_.saturatorCurve = previous.saturatorCurve;
    current_.saturatorSplit = previous.saturatorSplit;
    current_.compressorAttack = previous.compressorAttack;
    current_.compressorSustain = previous.compressorSustain;
    current_.compressorCompression = previous.compressorCompression;
    current_.compressorCharacter = previous.compressorCharacter;
    current_.compressorParallel = previous.compressorParallel;
    current_.compressorOutputDb = previous.compressorOutputDb;
    current_.polishThick = previous.polishThick;
    current_.polishAir = previous.polishAir;
    current_.outputLevel = previous.outputLevel;
    current_.outputTarget = previous.outputTarget;

    target_ = target;
    remainingSamples_ = static_cast<std::size_t>(
        std::max(1.0, std::round(sampleRate_ * kRampDurationSeconds)));
    const auto divisor = static_cast<float>(remainingSamples_);

    step_.inputLevel = (target_.inputLevel - current_.inputLevel) / divisor;
    step_.inputTarget = (target_.inputTarget - current_.inputTarget) / divisor;
    step_.inputWidth = (target_.inputWidth - current_.inputWidth) / divisor;
    step_.saturatorDrive =
        (target_.saturatorDrive - current_.saturatorDrive) / divisor;
    step_.saturatorOutputDb =
        (target_.saturatorOutputDb - current_.saturatorOutputDb) / divisor;
    step_.saturatorCurve =
        (target_.saturatorCurve - current_.saturatorCurve) / divisor;
    step_.compressorAttack = (target_.compressorAttack - current_.compressorAttack) / divisor;
    step_.compressorSustain = (target_.compressorSustain - current_.compressorSustain) / divisor;
    step_.compressorCompression = (target_.compressorCompression - current_.compressorCompression) / divisor;
    step_.compressorOutputDb =
        (target_.compressorOutputDb - current_.compressorOutputDb) / divisor;
    step_.polishThick = (target_.polishThick - current_.polishThick) / divisor;
    step_.polishAir = (target_.polishAir - current_.polishAir) / divisor;
    step_.outputLevel = (target_.outputLevel - current_.outputLevel) / divisor;
    step_.outputTarget = (target_.outputTarget - current_.outputTarget) / divisor;
}

const DspSnapshot& DspParameterSmoother::Advance() noexcept
{
    if (remainingSamples_ == 0)
    {
        return current_;
    }

    current_.inputLevel += step_.inputLevel;
    current_.inputTarget += step_.inputTarget;
    current_.inputWidth += step_.inputWidth;
    current_.saturatorDrive += step_.saturatorDrive;
    current_.saturatorOutputDb += step_.saturatorOutputDb;
    current_.saturatorCurve += step_.saturatorCurve;
    current_.compressorAttack += step_.compressorAttack;
    current_.compressorSustain += step_.compressorSustain;
    current_.compressorCompression += step_.compressorCompression;
    current_.compressorOutputDb += step_.compressorOutputDb;
    current_.polishThick += step_.polishThick;
    current_.polishAir += step_.polishAir;
    current_.outputLevel += step_.outputLevel;
    current_.outputTarget += step_.outputTarget;
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
