#include "DetectorEqualizer.h"

#include <algorithm>
#include <cstddef>

namespace consolidator::audio::devices
{

void DetectorEqualizer::Prepare(const ProcessingContext& context)
{
    left_.assign(context.maximumFrameCount, 0.0);
    right_.assign(context.maximumFrameCount, 0.0);
    for (auto& filter : filters_)
    {
        filter.Prepare(context);
    }
}

void DetectorEqualizer::Reset() noexcept
{
    for (auto& filter : filters_)
    {
        filter.Reset();
    }
}

void DetectorEqualizer::SetParameters(const std::array<FilterParameters, 2>& filters) noexcept
{
    for (std::size_t index = 0; index < filters_.size(); ++index)
    {
        filters_[index].SetParameters(filters[index]);
    }
}

DetectorBuffer DetectorEqualizer::Capture(AudioBuffer audio, ChannelMode mode) noexcept
{
    // Runtime validates capacity once before calling any device.
    if (audio.left != nullptr)
    {
        std::copy_n(audio.left, audio.frameCount, left_.data());
    }
    if (audio.right != nullptr)
    {
        std::copy_n(audio.right, audio.frameCount, right_.data());
    }
    AudioBuffer detector{
        audio.left != nullptr ? left_.data() : nullptr,
        audio.right != nullptr ? right_.data() : nullptr,
        audio.frameCount };
    for (auto& filter : filters_)
    {
        filter.Process(detector, mode);
    }
    return { detector.left, detector.right };
}

} // namespace consolidator::audio::devices
