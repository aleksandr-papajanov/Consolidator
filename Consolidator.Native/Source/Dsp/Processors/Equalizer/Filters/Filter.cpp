#include "Dsp/Processors/Equalizer/Filters/Filter.h"
#include <algorithm>
#include <cmath>
#include "Dsp/Parameters/ParameterHelper.h"
namespace consolidator::dsp
{
Filter::Filter(DeviceId deviceId, detail::ElementKind elementKind, std::uint8_t elementIndex)
    : deviceId_(deviceId), elementKind_(elementKind), elementIndex_(elementIndex) {}
void Filter::Prepare(double sampleRate, std::size_t channelCount) { sampleRate_ = std::max(sampleRate, 1.0); activeChannelCount_ = std::clamp<std::size_t>(channelCount, 1, kMaximumChannelCount); }
void Filter::Reset() noexcept { for (auto& s : channelStates_) { s.z1 = 0.0; s.z2 = 0.0; } }
void Filter::Process(const double* input, double* output, std::size_t frameCount, std::size_t channelCount)
{
    for (std::size_t frame = 0; frame < frameCount; ++frame)
        for (std::size_t ch = 0; ch < std::min(channelCount, activeChannelCount_); ++ch)
            output[frame * channelCount + ch] = ProcessSample(input[frame * channelCount + ch], ch);
}
void Filter::ApplyParameterChange(const ParameterChange& change)
{
    switch (change.address.GetParameterId())
    {
    case ParameterId::Frequency: if (auto* v = TryGetValue<float>(change)) SetFrequency(*v); break;
    case ParameterId::Q: if (auto* v = TryGetValue<float>(change)) SetQ(*v); break;
    case ParameterId::Gain: if (auto* v = TryGetValue<float>(change)) SetGain(*v); break;
    case ParameterId::Bypass: if (auto* v = TryGetValue<bool>(change)) SetBypass(*v); break;
    default: break;
    }
}
double Filter::ProcessSample(double input, std::size_t channel) noexcept
{
    if (channel >= channelStates_.size()) return input;
    return ProcessActiveSample(input, channel);
}
double Filter::ProcessActiveSample(double input, std::size_t channel) noexcept
{
    auto& m = channelStates_[channel];
    const double out = coefficients_.b0 * input + m.z1;
    m.z1 = coefficients_.b1 * input - coefficients_.a1 * out + m.z2;
    m.z2 = coefficients_.b2 * input - coefficients_.a2 * out;
    return out;
}
void Filter::SetFrequency(float f) { parameters_.frequencyHz = std::clamp(static_cast<double>(f), kMinimumFrequencyHz, sampleRate_ * 0.49); RecalculateCoefficients(); }
void Filter::SetQ(float q) { parameters_.q = std::max(static_cast<double>(q), kMinimumQ); RecalculateCoefficients(); }
void Filter::SetGain(float g) { parameters_.gainDb = static_cast<double>(g); RecalculateCoefficients(); }
void Filter::SetBypass(bool b) noexcept { parameters_.bypass = b; SetNeutral(b); }
double Filter::GetMaximumFrequencyHz() const noexcept { return sampleRate_ * 0.49; }
} // namespace consolidator::dsp