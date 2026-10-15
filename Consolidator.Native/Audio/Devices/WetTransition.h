#pragma once

#include <algorithm>
#include <cstddef>
#include <vector>

#include "../Primitives/ChannelView.h"
#include "../Primitives/ParameterRamp.h"

namespace consolidator::audio::devices
{

// Device-owned bypass/parallel transition. No virtual processor hierarchy is required.
// The wet callback must touch only the borrowed buffer; its detector route stays local.
class WetTransition final
{
public:
    void Prepare(const ProcessingContext& context)
    {
        left_.assign(context.maximumFrameCount, 0.0);
        right_.assign(context.maximumFrameCount, 0.0);
        wet_.Prepare(context);
    }

    void Reset() noexcept
    {
        wet_.Reset();
        wasProcessing_ = false;
    }

    void SetTarget(double wet) noexcept
    {
        wet_.SetTarget(wet);
    }

    template <typename ProcessWet, typename ResetWet>
    void Process(AudioBuffer audio, ChannelMode mode, ProcessWet&& processWet, ResetWet&& resetWet) noexcept
    {
        if (wet_.CurrentValue() == 0.0 && !wet_.IsSmoothing())
        {
            if (wasProcessing_)
            {
                resetWet();
                wasProcessing_ = false;
            }
            return;
        }
        wasProcessing_ = true;
        if (wet_.CurrentValue() == 1.0 && !wet_.IsSmoothing())
        {
            processWet(audio);
            return;
        }
        if (audio.left != nullptr)
        {
            std::copy_n(audio.left, audio.frameCount, left_.data());
        }
        if (audio.right != nullptr)
        {
            std::copy_n(audio.right, audio.frameCount, right_.data());
        }
        AudioBuffer branch{
            audio.left != nullptr ? left_.data() : nullptr,
            audio.right != nullptr ? right_.data() : nullptr,
            audio.frameCount };
        processWet(branch);
        const auto channels = primitives::SelectChannels(audio, mode);
        for (std::size_t index = 0; index < audio.frameCount; ++index)
        {
            const auto wet = wet_.NextValue();
            if (channels.left != nullptr)
            {
                channels.left[index] += wet * (branch.left[index] - channels.left[index]);
            }
            if (channels.right != nullptr)
            {
                channels.right[index] += wet * (branch.right[index] - channels.right[index]);
            }
        }
    }

private:
    primitives::ParameterRamp wet_;
    std::vector<double> left_;
    std::vector<double> right_;
    bool wasProcessing_{};
};

} // namespace consolidator::audio::devices
