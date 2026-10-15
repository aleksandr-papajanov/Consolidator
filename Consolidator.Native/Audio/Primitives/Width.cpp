#include "Width.h"

#include <cstddef>

namespace consolidator::audio::primitives
{

void Width::Prepare(const ProcessingContext& context) noexcept
{
    amount_.Prepare(context);
}

void Width::Reset() noexcept
{
    amount_.Reset();
}

void Width::SetAmount(double width) noexcept
{
    amount_.SetTarget(width);
}

void Width::Process(AudioBuffer audio, ChannelMode mode) noexcept
{
    if (mode != ChannelMode::Stereo || audio.left == nullptr || audio.right == nullptr)
    {
        amount_.Advance(audio.frameCount);
        return;
    }
    for (std::size_t index = 0; index < audio.frameCount; ++index)
    {
        const auto width = amount_.NextValue();
        const auto middle = (audio.left[index] + audio.right[index]) * 0.5;
        const auto side = (audio.left[index] - audio.right[index]) * 0.5 * width;
        audio.left[index] = middle + side;
        audio.right[index] = middle - side;
    }
}

} // namespace consolidator::audio::primitives
