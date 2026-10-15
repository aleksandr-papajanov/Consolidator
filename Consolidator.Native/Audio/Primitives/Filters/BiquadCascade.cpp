#include "BiquadCascade.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace consolidator::audio::primitives::filters
{

namespace
{

constexpr double kTransitionSeconds = 0.01;
constexpr double kMinimumQ = 1.0e-6;
constexpr double kNyquistLimit = 0.499;

} // namespace

void BiquadCascade::Prepare(
    const ProcessingContext& context) noexcept
{
    sampleRate_ = std::isfinite(context.sampleRate) && context.sampleRate > 0.0
        ? context.sampleRate
        : 48000.0;
    transitionLength_ = std::max<std::size_t>(
        1,
        static_cast<std::size_t>(sampleRate_ * kTransitionSeconds));
    if (initialized_)
    {
        // A new sample rate invalidates coefficients even when physical controls are unchanged.
        current_ = Compile(parameters_, enabled_);
        target_ = current_;
        transitionPosition_ = transitionLength_;
    }
}

void BiquadCascade::Reset() noexcept
{
    current_ = target_;
    ClearState(current_);
    ClearState(target_);
    transitionPosition_ = transitionLength_;
}

void BiquadCascade::SetParameters(
    const FilterParameters& parameters,
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

void BiquadCascade::Process(
    AudioBuffer& audio,
    ChannelMode channelMode) noexcept
{
    if (!RequiresProcessing())
    {
        return;
    }

    const auto processLeft = channelMode != ChannelMode::Right &&
        audio.left != nullptr;
    const auto processRight = channelMode != ChannelMode::Left &&
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

bool BiquadCascade::RequiresProcessing() const noexcept
{
    return initialized_ &&
        (current_.count > 0 ||
        target_.count > 0 ||
        transitionPosition_ < transitionLength_);
}

BiquadCascade::Path BiquadCascade::Compile(
    const FilterParameters& parameters,
    bool enabled) const noexcept
{
    Path path{};
    if (!enabled || parameters.active == 0 ||
        !std::isfinite(parameters.gainDb) ||
        parameters.gainDb == 0.0F)
    {
        return path;
    }

    const auto type = static_cast<FilterType>(parameters.type);
    if (type == FilterType::Gain)
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
    const auto qValue = type == FilterType::Bell
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
        case FilterType::Tilt:
        {
            path.count = 2;
            path.stages[0].coefficients = DesignShelf(
                true, frequency, q, -gain, sampleRate_);
            path.stages[1].coefficients = DesignShelf(
                false, frequency, q, gain, sampleRate_);
            break;
        }
        case FilterType::LowShelf:
        {
            path.count = 1;
            path.stages[0].coefficients = DesignShelf(
                true, frequency, q, gain, sampleRate_);
            break;
        }
        case FilterType::HighShelf:
        {
            path.count = 1;
            path.stages[0].coefficients = DesignShelf(
                false, frequency, q, gain, sampleRate_);
            break;
        }
        case FilterType::Bell:
        {
            path.count = 1;
            path.stages[0].coefficients = DesignBell(
                frequency, q, gain, sampleRate_);
            break;
        }
        case FilterType::None:
        case FilterType::Gain:
        default:
        {
            break;
        }
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

double BiquadCascade::ProcessPath(
    Path& path,
    double input,
    bool rightChannel) noexcept
{
    auto output = input;
    for (std::size_t index = 0; index < path.count; ++index)
    {
        output = path.stages[index].Process(output, rightChannel);
    }
    return output;
}

bool BiquadCascade::ParametersEqual(
    const FilterParameters& left,
    const FilterParameters& right) noexcept
{
    return left.active == right.active &&
        left.type == right.type &&
        left.frequencyHz == right.frequencyHz &&
        left.gainDb == right.gainDb &&
        left.q == right.q &&
        left.fixedQ == right.fixedQ;
}

void BiquadCascade::ClearState(Path& path) noexcept
{
    for (auto& stage : path.stages)
    {
        stage.Reset();
    }
}

} // namespace consolidator::audio::primitives::filters
