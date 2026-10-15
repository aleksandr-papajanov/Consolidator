#include "Shared/Controls/Parameter/ViewModels/ParameterViewModel.h"

#include <stdexcept>
#include <utility>

namespace consolidator::vst3::shared::controls::parameter::viewmodels
{

ParameterViewModel::ParameterViewModel(shared::clients::ParameterEntry entry,
    const shared::controls::dial::DialRuntimeOverlayViewModel* runtimeOverlay,
    std::span<const std::string_view> choices)
    : state_(std::move(entry)), choices_(choices)
{
    if (std::holds_alternative<double>(state_.Entry().value))
    {
        dial_ = std::make_unique<shared::controls::dial::DialViewModel>(state_, runtimeOverlay);
    }
    else
    {
        ValidateDiscrete();
    }
}

bool ParameterViewModel::Apply(shared::clients::ParameterEntry entry)
{
    const bool changed = state_.Apply(std::move(entry));
    if (dial_)
    {
        dial_->Validate();
    }
    else
    {
        ValidateDiscrete();
    }
    return changed;
}

void ParameterViewModel::ValidateDiscrete() const
{
    if (choices_.empty())
    {
        (void)shared::protocol::Boolean(state_.Entry().value);
    }
    else
    {
        const auto value = shared::protocol::Integer(state_.Entry().value);
        if (value < 0 || static_cast<std::size_t>(value) >= choices_.size())
        {
            throw std::invalid_argument("Invalid parameter choice.");
        }
    }
}

shared::presentation::StateValueViewModel& ParameterViewModel::State() { return state_; }
const shared::presentation::StateValueViewModel& ParameterViewModel::State() const { return state_; }
shared::controls::dial::DialViewModel* ParameterViewModel::Dial() { return dial_.get(); }
bool ParameterViewModel::HasChoices() const { return !choices_.empty(); }
int ParameterViewModel::ChoiceValue() const { return static_cast<int>(shared::protocol::Integer(state_.Entry().value)); }
std::span<const std::string_view> ParameterViewModel::Choices() const { return choices_; }
bool ParameterViewModel::ButtonValue() const { return shared::protocol::Boolean(state_.Entry().value); }
bool ParameterViewModel::GroupScope() const { return groupScope_; }

void ParameterViewModel::SetScope(bool groupScope)
{
    groupScope_ = groupScope;
    if (dial_)
    {
        dial_->SetScope(groupScope);
    }
}

} // namespace consolidator::vst3::shared::controls::parameter::viewmodels
