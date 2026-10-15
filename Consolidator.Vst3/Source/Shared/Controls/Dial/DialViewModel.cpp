#include "Shared/Controls/Dial/DialViewModel.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <stdexcept>

namespace consolidator::vst3::shared::controls::dial
{

DialViewModel::DialViewModel(const shared::presentation::StateValueViewModel& state,
    const DialRuntimeOverlayViewModel* runtimeOverlay)
    : state_(state), runtimeOverlay_(runtimeOverlay)
{
    const auto last = state.Path().substr(state.Path().rfind('.') + 1);
    logarithmic_ = last == "frequency";
    decibels_ = last == "gain" || last == "level" || last == "target" || last == "output";
    if (decibels_)
    {
        suffix_ = " dB";
    }
    else if (logarithmic_)
    {
        suffix_ = " Hz";
        decimals_ = 0;
    }
    else if (last == "q")
    {
        decimals_ = 2;
    }
    else
    {
        displayScale_ = 100;
        suffix_ = "%";
    }
    Validate();
}

const shared::presentation::StateValueViewModel& DialViewModel::State() const { return state_; }
bool DialViewModel::Automated() const { return runtimeOverlay_ != nullptr && runtimeOverlay_->Enabled(); }
bool DialViewModel::GroupScope() const { return groupScope_; }
bool DialViewModel::Previewing() const { return preview_.has_value(); }
void DialViewModel::SetPreview(double value) { preview_ = value; }
void DialViewModel::FinishPreview() { preview_.reset(); }

void DialViewModel::SetScope(bool groupScope)
{
    groupScope_ = groupScope;
    Validate();
}

void DialViewModel::Validate() const
{
    const auto& entry = state_.Entry();
    if (!entry.minimum || !entry.maximum || *entry.maximum < *entry.minimum ||
        !std::isfinite(shared::protocol::Number(entry.value)) || (logarithmic_ && *entry.minimum <= 0))
    {
        throw std::invalid_argument("Invalid dial physical metadata: " + entry.path);
    }
    const auto range = EditingRange();
    if (range.first < *entry.minimum || range.second > *entry.maximum || range.first > range.second)
    {
        throw std::invalid_argument("Invalid dial effective range: " + entry.path);
    }
}

std::pair<double, double> DialViewModel::EditingRange() const
{
    const auto& entry = state_.Entry();
    if (groupScope_)
    {
        if (!entry.effectiveMinimum || !entry.effectiveMaximum)
        {
            throw std::invalid_argument("Missing dial group metadata: " + entry.path);
        }
        return { *entry.effectiveMinimum, *entry.effectiveMaximum };
    }
    return { *entry.minimum, *entry.maximum };
}

bool DialViewModel::Editable() const
{
    const auto range = EditingRange();
    return state_.Enabled() && !Automated() && range.first < range.second;
}

double DialViewModel::Normalize(double value) const
{
    const auto& entry = state_.Entry();
    const double minimum = *entry.minimum;
    const double maximum = *entry.maximum;
    if (minimum == maximum)
    {
        return 0;
    }
    value = std::clamp(value, minimum, maximum);
    return logarithmic_ ? std::log(value / minimum) / std::log(maximum / minimum)
        : (value - minimum) / (maximum - minimum);
}

double DialViewModel::Denormalize(double value) const
{
    const auto& entry = state_.Entry();
    return logarithmic_ ? *entry.minimum * std::pow(*entry.maximum / *entry.minimum, value)
        : *entry.minimum + (*entry.maximum - *entry.minimum) * value;
}

double DialViewModel::Quantize(double normalizedPosition, bool rough) const
{
    const auto range = EditingRange();
    double value = Denormalize(normalizedPosition);
    if (rough)
    {
        const double step = decibels_ ? 3 : 0.1;
        value = std::floor(value / step + 0.5) * step;
    }
    return std::clamp(value, range.first, range.second);
}

double DialViewModel::DisplayValue() const
{
    if (Automated())
    {
        if (const auto displayValue = runtimeOverlay_->DisplayValue())
        {
            return *displayValue;
        }
    }
    return preview_ ? *preview_ : shared::protocol::Number(state_.Entry().value);
}

std::pair<double, double> DialViewModel::RingRange() const
{
    const auto range = EditingRange();
    return groupScope_ ? std::pair{ Normalize(range.first), Normalize(range.second) } : std::pair{ 0.0, 1.0 };
}

double DialViewModel::RingValue() const
{
    const auto range = RingRange();
    return std::clamp(Normalize(DisplayValue()), range.first, range.second);
}

std::optional<double> DialViewModel::FastPosition() const
{
    return runtimeOverlay_ != nullptr ? runtimeOverlay_->FastPosition() : std::optional<double>{};
}

std::string DialViewModel::ValueText() const
{
    std::array<char, 64> buffer;
    const auto result = std::to_chars(buffer.data(), buffer.data() + buffer.size(),
        DisplayValue() * displayScale_, std::chars_format::fixed, decimals_);
    if (result.ec != std::errc{})
    {
        throw std::invalid_argument("Cannot format dial value: " + state_.Path());
    }
    return std::string(buffer.data(), result.ptr) + suffix_;
}

} // namespace consolidator::vst3::shared::controls::dial
