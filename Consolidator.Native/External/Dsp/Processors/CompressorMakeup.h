#pragma once

#include <algorithm>
#include <cmath>

#include "../DspMath.h"

namespace consolidator::max::dsp::processors
{

// Mean attenuation of eight unnormalised musical sources. This is a settings law,
// not a signal follower; source-specific differences remain. See Consolidator.Native/Docs/Aspects/ProgramMakeup.md.
inline double ProgramReductionDb(double reduction, double onset, double width, double slope) noexcept
{
    const auto position = std::max(reduction - onset, 0.0);
    return slope * position * position / (position + width);
}

inline double Cla3aProgramMakeupGain(double reduction) noexcept
{
    return DbToGain(ProgramReductionDb(reduction, 0.397, 0.198, 37.078));
}

inline double Cla2aProgramMakeupGain(double reduction) noexcept
{
    return DbToGain(ProgramReductionDb(reduction, 0.413, 0.127, 36.471));
}

// ratioPosition is the internal 1:1-to-ALL coordinate, not the UI ratio taper.
inline double Cla76ProgramMakeupGain(
    double reduction, double attack, double release, double ratioPosition) noexcept
{
    const auto inverseRatio = 1.0 / (1.0 + 99.0 * ratioPosition);
    const auto strength = 1.0 - inverseRatio;
    if (reduction == 0.0 || strength == 0.0)
    {
        return 1.0;
    }
    const auto all = std::max((99.0 * ratioPosition - 3.0) / 96.0, 0.0);
    const auto attackPosition = attack - 0.5;
    const auto releasePosition = release - 0.5;
    const auto logSlope = 3.876 - 0.015 * attackPosition + 0.036 * releasePosition -
        0.108 * attackPosition * attackPosition + 0.019 * attackPosition * releasePosition +
        all * (0.192 + 0.085 * attackPosition + 0.187 * releasePosition) +
        inverseRatio * (1.324 - 1.295 * inverseRatio);
    const auto slope = std::exp(logSlope) * strength;
    const auto onset = 0.408 + 0.051 * attackPosition - 0.047 * releasePosition +
        0.065 * all - 0.011 * inverseRatio;
    return DbToGain(ProgramReductionDb(reduction, onset, 0.140, slope));
}

inline double SslProgramMakeupGain(
    double reduction, bool fastAttack, double releaseSeconds, double ratio) noexcept
{
    const auto inverseRatio = 1.0 / ratio;
    const auto strength = 1.0 - inverseRatio;
    if (reduction == 0.0 || strength == 0.0)
    {
        return 1.0;
    }
    const auto attackPosition = fastAttack ? -0.5 : 0.5;
    const auto releasePosition = std::log(releaseSeconds / 0.4) / 3.0;
    const auto logSlope = 3.969 - 0.376 * attackPosition + 0.188 * releasePosition +
        0.156 * attackPosition * releasePosition - inverseRatio * (0.137 + 0.463 * inverseRatio);
    const auto slope = std::exp(logSlope) * strength;
    const auto onset = 0.417 + 0.070 * attackPosition - 0.040 * releasePosition + 0.049 * inverseRatio;
    return DbToGain(ProgramReductionDb(reduction, onset, 0.467, slope));
}

} // namespace consolidator::max::dsp::processors
