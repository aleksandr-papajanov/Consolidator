#include "DspBiquad.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace consolidator::max::dsp
{

namespace
{

constexpr double kPi = 3.141592653589793238462643383279502884;
constexpr double kTransitionSeconds = 0.01;
constexpr double kMinimumQ = 1.0e-6;
constexpr double kNyquistLimit = 0.499;
constexpr double kDenormalThreshold = 1.0e-24;

[[nodiscard]] double SanitizeState(double value) noexcept
{
    return std::abs(value) < kDenormalThreshold ? 0.0 : value;
}

} // namespace

void DspBiquadCascade::Prepare(
    const DspProcessingContext& context) noexcept
{
    sampleRate_ = std::isfinite(context.sampleRate) && context.sampleRate > 0.0
        ? context.sampleRate
        : 48000.0;
    transitionLength_ = std::max<std::size_t>(
        1,
        static_cast<std::size_t>(sampleRate_ * kTransitionSeconds));
}

void DspBiquadCascade::Reset() noexcept
{
    current_ = target_;
    ClearState(current_);
    ClearState(target_);
    transitionPosition_ = transitionLength_;
}

void DspBiquadCascade::SetParameters(
    const FilterSnapshot& parameters,
    bool enabled) noexcept
{
    if (initialized_ && enabled_ == enabled &&
        ParametersEqual(parameters_, parameters))
    {
        return;
    }

    parameters_ = parameters;
    enabled_ = enabled;
    auto next = Compile(parameters, enabled);
    if (!initialized_)
    {
        current_ = next;
        target_ = next;
        initialized_ = true;
        transitionPosition_ = transitionLength_;
        return;
    }

    if (transitionPosition_ < transitionLength_)
    {
        current_ = target_;
    }
    target_ = next;
    transitionPosition_ = 0;
}

void DspBiquadCascade::Process(
    DspAudioBuffer& audio,
    DspChannelMode channelMode) noexcept
{
    if (!RequiresProcessing())
    {
        return;
    }

    const auto processLeft = channelMode != DspChannelMode::Right &&
        audio.left != nullptr;
    const auto processRight = channelMode != DspChannelMode::Left &&
        audio.right != nullptr;
    if (transitionPosition_ >= transitionLength_)
    {
        // Settled coefficients need only the current cascade, without the
        // transition ratio and identity interpolation on every sample.
        for (std::size_t index = 0; index < audio.frameCount; ++index)
        {
            if (processLeft)
            {
                audio.left[index] = ProcessPath(current_, audio.left[index], false);
            }
            if (processRight)
            {
                audio.right[index] = ProcessPath(current_, audio.right[index], true);
            }
        }
        return;
    }
    for (std::size_t index = 0; index < audio.frameCount; ++index)
    {
        const auto transitioning = transitionPosition_ < transitionLength_;
        const auto mix = transitioning
            ? static_cast<double>(transitionPosition_) /
                static_cast<double>(transitionLength_)
            : 1.0;
        if (processLeft)
        {
            const auto input = audio.left[index];
            const auto current = ProcessPath(current_, input, false);
            const auto target = transitioning
                ? ProcessPath(target_, input, false)
                : current;
            audio.left[index] = current + (target - current) * mix;
        }
        if (processRight)
        {
            const auto input = audio.right[index];
            const auto current = ProcessPath(current_, input, true);
            const auto target = transitioning
                ? ProcessPath(target_, input, true)
                : current;
            audio.right[index] = current + (target - current) * mix;
        }
        if (transitioning && ++transitionPosition_ >= transitionLength_)
        {
            current_ = target_;
        }
    }
}

bool DspBiquadCascade::RequiresProcessing() const noexcept
{
    return initialized_ &&
        (current_.count > 0 ||
        target_.count > 0 ||
        transitionPosition_ < transitionLength_);
}

DspBiquadCascade::Path DspBiquadCascade::Compile(
    const FilterSnapshot& parameters,
    bool enabled) const noexcept
{
    Path path{};
    if (!enabled || parameters.active == 0 ||
        !std::isfinite(parameters.gainDb) ||
        parameters.gainDb == 0.0F)
    {
        return path;
    }

    const auto type = static_cast<DspFilterType>(parameters.type);
    if (type == DspFilterType::Gain)
    {
        path.count = 1;
        path.stages[0].coefficients.b0 =
            std::pow(10.0, static_cast<double>(parameters.gainDb) / 20.0);
        return path;
    }

    if (!std::isfinite(parameters.frequencyHz) ||
        parameters.frequencyHz <= 0.0F)
    {
        return path;
    }

    const auto frequency = std::min(
        static_cast<double>(parameters.frequencyHz),
        sampleRate_ * kNyquistLimit);
    const auto qValue = type == DspFilterType::Bell
        ? parameters.q
        : parameters.fixedQ;
    if (!std::isfinite(qValue) || qValue <= 0.0F)
    {
        return path;
    }
    const auto q = std::max(static_cast<double>(qValue), kMinimumQ);
    const auto gain = static_cast<double>(parameters.gainDb);
    switch (type)
    {
        case DspFilterType::Tilt:
            path.count = 2;
            path.stages[0].coefficients = CompileShelf(
                true, frequency, q, -gain, sampleRate_);
            path.stages[1].coefficients = CompileShelf(
                false, frequency, q, gain, sampleRate_);
            break;
        case DspFilterType::LowShelf:
            path.count = 1;
            path.stages[0].coefficients = CompileShelf(
                true, frequency, q, gain, sampleRate_);
            break;
        case DspFilterType::HighShelf:
            path.count = 1;
            path.stages[0].coefficients = CompileShelf(
                false, frequency, q, gain, sampleRate_);
            break;
        case DspFilterType::Bell:
            path.count = 1;
            path.stages[0].coefficients = CompileBell(
                frequency, q, gain, sampleRate_);
            break;
        case DspFilterType::None:
        case DspFilterType::Gain:
        default:
            break;
    }

    for (std::size_t index = 0; index < path.count; ++index)
    {
        const auto& coefficients = path.stages[index].coefficients;
        if (!std::isfinite(coefficients.b0) ||
            !std::isfinite(coefficients.b1) ||
            !std::isfinite(coefficients.b2) ||
            !std::isfinite(coefficients.a1) ||
            !std::isfinite(coefficients.a2))
        {
            return {};
        }
    }
    return path;
}

DspBiquadCascade::Coefficients DspBiquadCascade::CompileBell(
    double frequencyHz,
    double q,
    double gainDb,
    double sampleRate) noexcept
{
    const auto omega = 2.0 * kPi * frequencyHz / sampleRate;
    const auto sine = std::sin(omega);
    const auto cosine = std::cos(omega);
    const auto alpha = sine / (2.0 * q);
    const auto amplitude = std::pow(10.0, gainDb / 40.0);
    const auto inverseA0 = 1.0 / (1.0 + alpha / amplitude);
    return {
        (1.0 + alpha * amplitude) * inverseA0,
        -2.0 * cosine * inverseA0,
        (1.0 - alpha * amplitude) * inverseA0,
        -2.0 * cosine * inverseA0,
        (1.0 - alpha / amplitude) * inverseA0
    };
}

DspBiquadCascade::Coefficients DspBiquadCascade::CompileShelf(
    bool lowShelf,
    double frequencyHz,
    double q,
    double gainDb,
    double sampleRate) noexcept
{
    const auto omega = 2.0 * kPi * frequencyHz / sampleRate;
    const auto sine = std::sin(omega);
    const auto cosine = std::cos(omega);
    const auto amplitude = std::pow(10.0, gainDb / 40.0);
    const auto alpha = sine / (2.0 * q);
    const auto beta = 2.0 * std::sqrt(amplitude) * alpha;
    const auto b0 = lowShelf
        ? amplitude * ((amplitude + 1.0) -
            (amplitude - 1.0) * cosine + beta)
        : amplitude * ((amplitude + 1.0) +
            (amplitude - 1.0) * cosine + beta);
    const auto b1 = lowShelf
        ? 2.0 * amplitude * ((amplitude - 1.0) -
            (amplitude + 1.0) * cosine)
        : -2.0 * amplitude * ((amplitude - 1.0) +
            (amplitude + 1.0) * cosine);
    const auto b2 = lowShelf
        ? amplitude * ((amplitude + 1.0) -
            (amplitude - 1.0) * cosine - beta)
        : amplitude * ((amplitude + 1.0) +
            (amplitude - 1.0) * cosine - beta);
    const auto a0 = lowShelf
        ? (amplitude + 1.0) + (amplitude - 1.0) * cosine + beta
        : (amplitude + 1.0) - (amplitude - 1.0) * cosine + beta;
    const auto a1 = lowShelf
        ? -2.0 * ((amplitude - 1.0) +
            (amplitude + 1.0) * cosine)
        : 2.0 * ((amplitude - 1.0) -
            (amplitude + 1.0) * cosine);
    const auto a2 = lowShelf
        ? (amplitude + 1.0) + (amplitude - 1.0) * cosine - beta
        : (amplitude + 1.0) - (amplitude - 1.0) * cosine - beta;
    const auto inverseA0 = 1.0 / a0;
    return {
        b0 * inverseA0,
        b1 * inverseA0,
        b2 * inverseA0,
        a1 * inverseA0,
        a2 * inverseA0
    };
}

double DspBiquadCascade::ProcessPath(
    Path& path,
    double input,
    bool rightChannel) noexcept
{
    auto output = input;
    for (std::size_t index = 0; index < path.count; ++index)
    {
        auto& stage = path.stages[index];
        auto& state = rightChannel ? stage.right : stage.left;
        const auto next = stage.coefficients.b0 * output + state.z1;
        state.z1 = SanitizeState(
            stage.coefficients.b1 * output -
            stage.coefficients.a1 * next + state.z2);
        state.z2 = SanitizeState(
            stage.coefficients.b2 * output -
            stage.coefficients.a2 * next);
        output = next;
    }
    return output;
}

bool DspBiquadCascade::ParametersEqual(
    const FilterSnapshot& left,
    const FilterSnapshot& right) noexcept
{
    return left.active == right.active &&
        left.type == right.type &&
        left.frequencyHz == right.frequencyHz &&
        left.gainDb == right.gainDb &&
        left.q == right.q &&
        left.fixedQ == right.fixedQ;
}

void DspBiquadCascade::ClearState(Path& path) noexcept
{
    for (auto& stage : path.stages)
    {
        stage.left = {};
        stage.right = {};
    }
}

} // namespace consolidator::max::dsp
