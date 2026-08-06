#include "Dsp/Processors/Equalizer/Equalizer.h"

namespace consolidator::dsp
{

void Equalizer::Process(
    const double* input,
    double* output,
    std::size_t frameCount,
    std::size_t channelCount)
{
    if (filters_.empty())
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

void Equalizer::ApplyParameterChange(
    const ParameterChange& change)
{
    if (change.address.GetBankId() != bankId_)
    {
        return;
    }

    if (change.address.GetElementKind() != detail::ElementKind::Device)
    {
        if (auto* f = FindFilter(
                change.address.GetElementKind(),
                change.address.GetElementIndex()))
        {
            f->ApplyParameterChange(change);
        }
        return;
    }

    for (auto& f : filters_)
    {
        f->ApplyParameterChange(change);
    }
}

void Equalizer::AddFilter(std::unique_ptr<Filter> filter)
{
    if (filter)
    {
        filters_.push_back(std::move(filter));
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
        if (f->GetElementKind() == elementKind &&
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
        if (f->GetElementKind() == elementKind &&
            f->GetElementIndex() == elementIndex)
        {
            return f.get();
        }
    }
    return nullptr;
}

} // namespace consolidator::dsp