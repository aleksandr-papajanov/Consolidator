#include "Dsp/Processors/Equalizer/Equalizer.h"


namespace consolidator::dsp
{

void Equalizer::Process(
    const double* input,
    double* output,
    std::size_t frameCount,
    std::size_t channelCount)
{
    if (IsNeutral())
    {
        const auto n = frameCount * channelCount;
        for (std::size_t i = 0; i < n; ++i)
        {
            output[i] = input[i];
        }
        return;
    }

    for (std::size_t frame = 0; frame < frameCount; ++frame)
    {
        for (std::size_t ch = 0; ch < channelCount; ++ch)
        {
            double sample = input[frame * channelCount + ch];
            for (auto& f : filters_)
            {
                sample = f->ProcessSample(sample, ch);
            }
            output[frame * channelCount + ch] = sample;
        }
    }
}

void Equalizer::Prepare(double sampleRate, std::size_t channelCount)
{
    for (auto& filter : filters_)
    {
        filter->Prepare(sampleRate, channelCount);
    }

    Reset();
    RecalculateRuntime();
}

void Equalizer::Reset() noexcept
{
    for (auto& filter : filters_)
    {
        filter->Reset();
    }
}

double Equalizer::ProcessSample(double input) noexcept
{
    double output = input;

    for (auto& filter : filters_)
    {
        output = filter->ProcessSample(output, 0);
    }

    return output;
}

bool Equalizer::WriteParameter(
    const core::StatePath& route,
    const ParameterValue& value,
    std::size_t depth)
{
    if (route.GetDeviceId() != GetDeviceId())
    {
        return false;
    }

    if (depth == route.GetDepth())
    {
        return DspDevice::WriteParameter(route, value, depth);
    }

    const RouteNodeId node = route.GetNode(depth);
    const std::size_t filterOffset =
        static_cast<std::size_t>(RouteNodeId::Filter1);

    if (static_cast<std::size_t>(node) < filterOffset)
    {
        return WriteParameter(route, value, depth + 1);
    }

    const std::size_t filterIndex = static_cast<std::size_t>(node) - filterOffset;
    auto* filter = GetFilter(filterIndex);
    if (filter == nullptr)
    {
        return false;
    }

    const bool isUpdated = filter->WriteParameter(route, value, depth + 1);
    if (isUpdated)
    {
        RecalculateRuntime();
    }

    return isUpdated;
}

void Equalizer::AddFilter(std::unique_ptr<Filter> filter)
{
    if (filter != nullptr)
    {
        filters_.push_back(std::move(filter));
        RecalculateRuntime();
    }
}

void Equalizer::ReadState(const core::StatePath& path, core::StateSnapshot& snapshot) const
{
    if (!bankId_)
    {
        return;
    }

    const auto bankNode = static_cast<RouteNodeId>(
        static_cast<std::uint8_t>(RouteNodeId::Bank0) + detail::ToIndex(*bankId_));
    AppendParameter(path, snapshot, core::StatePath{DeviceId::Equalizer, ParameterId::Bypass, bankNode}, state_.bypass);

    for (const auto& filter : filters_)
    {
        filter->ReadAtRoute(path, snapshot, DeviceId::Equalizer, bankNode);
    }
}

void Equalizer::ReadAtRoute(
    const core::StatePath& path,
    core::StateSnapshot& snapshot,
    DeviceId deviceId,
    RouteNodeId parentNode) const
{
    for (const auto& filter : filters_)
    {
        filter->ReadAtRoute(path, snapshot, deviceId, parentNode);
    }
}

bool Equalizer::WriteOwnParameter(
    const core::StatePath& route,
    const ParameterValue& value)
{
    return state_.bypass.Apply(route, value);
}

void Equalizer::RecalculateRuntime()
{
    if (state_.bypass.value)
    {
        runtimeState_.isNeutral = true;
        return;
    }

    runtimeState_.isNeutral = true;
    for (const auto& filter : filters_)
    {
        if (!filter->IsNeutral())
        {
            runtimeState_.isNeutral = false;
            return;
        }
    }

}

Filter* Equalizer::GetFilter(std::size_t index) noexcept
{
    return index < filters_.size() ? filters_[index].get() : nullptr;
}

const Filter* Equalizer::GetFilter(std::size_t index) const noexcept
{
    return index < filters_.size() ? filters_[index].get() : nullptr;
}

Filter* Equalizer::FindFilter(
    detail::ElementKind elementKind,
    std::uint8_t elementIndex) noexcept
{
    for (auto& f : filters_)
    {
        if ((filterElementKind_ != detail::ElementKind::EqFilter ||
             f->GetElementKind() == elementKind) &&
             f->GetElementIndex() == elementIndex)
        {
            return f.get();
        }
    }
    return nullptr;
}

const Filter* Equalizer::FindFilter(
    detail::ElementKind elementKind,
    std::uint8_t elementIndex) const noexcept
{
    for (const auto& f : filters_)
    {
        if ((filterElementKind_ != detail::ElementKind::EqFilter ||
             f->GetElementKind() == elementKind) &&
             f->GetElementIndex() == elementIndex)
        {
            return f.get();
        }
    }
    return nullptr;
}

} // namespace consolidator::dsp
