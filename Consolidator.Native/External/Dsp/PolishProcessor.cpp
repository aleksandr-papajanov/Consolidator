#include "DspProcessors.h"

#include <algorithm>

namespace consolidator::max::dsp
{

namespace
{

void CopyChannel(
    const double* source,
    double* destination,
    std::size_t frameCount) noexcept
{
    if (source != nullptr)
    {
        std::copy_n(source, frameCount, destination);
    }
}

} // namespace

void PolishProcessor::Prepare(const DspProcessingContext& context)
{
    thick_.Prepare(context);
    air_.Prepare(context);
    thickLeft_.assign(context.maximumFrameCount, 0.0);
    thickRight_.assign(context.maximumFrameCount, 0.0);
    airLeft_.assign(context.maximumFrameCount, 0.0);
    airRight_.assign(context.maximumFrameCount, 0.0);
}

void PolishProcessor::Reset() noexcept
{
    thick_.Reset();
    air_.Reset();
    std::fill(thickLeft_.begin(), thickLeft_.end(), 0.0);
    std::fill(thickRight_.begin(), thickRight_.end(), 0.0);
    std::fill(airLeft_.begin(), airLeft_.end(), 0.0);
    std::fill(airRight_.begin(), airRight_.end(), 0.0);
}

void PolishProcessor::SetParameters(
    const DspSnapshot& snapshot) noexcept
{
    thick_.SetParameters(snapshot);
    air_.SetParameters(snapshot);
}

void PolishProcessor::Process(
    DspProcessingFrame& frame) noexcept
{
    const auto frameCount = frame.main.frameCount;
    if (frameCount > thickLeft_.size() ||
        frameCount > thickRight_.size() ||
        frameCount > airLeft_.size() ||
        frameCount > airRight_.size())
    {
        return;
    }

    CopyChannel(frame.main.left, thickLeft_.data(), frameCount);
    CopyChannel(frame.main.right, thickRight_.data(), frameCount);
    CopyChannel(frame.main.left, airLeft_.data(), frameCount);
    CopyChannel(frame.main.right, airRight_.data(), frameCount);

    DspAudioBuffer thickAudio{
        frame.main.left == nullptr ? nullptr : thickLeft_.data(),
        frame.main.right == nullptr ? nullptr : thickRight_.data(),
        frameCount
    };
    DspProcessingFrame thickFrame{
        thickAudio,
        frame.detectorSignal,
        frame.plan,
        frame.policy
    };
    thick_.Process(thickFrame);

    DspAudioBuffer airAudio{
        frame.main.left == nullptr ? nullptr : airLeft_.data(),
        frame.main.right == nullptr ? nullptr : airRight_.data(),
        frameCount
    };
    DspProcessingFrame airFrame{
        airAudio,
        frame.detectorSignal,
        frame.plan,
        frame.policy
    };
    air_.Process(airFrame);

    for (std::size_t index = 0; index < frameCount; ++index)
    {
        if (frame.main.left != nullptr)
        {
            frame.main.left[index] =
                (thickLeft_[index] + airLeft_[index]) * 0.5;
        }
        if (frame.main.right != nullptr)
        {
            frame.main.right[index] =
                (thickRight_[index] + airRight_[index]) * 0.5;
        }
    }
}

void PolishProcessor::SynchronizeInactiveState() noexcept
{
    thick_.Reset();
    air_.Reset();
}

} // namespace consolidator::max::dsp
