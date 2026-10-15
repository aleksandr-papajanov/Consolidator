#include "Shared/Presentation/StateValueViewModel.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <utility>

namespace consolidator::vst3::shared::presentation
{

StateValueViewModel::StateValueViewModel(shared::clients::ParameterEntry entry) : entry_(std::move(entry))
{
    label_ = entry_.path.substr(entry_.path.rfind('.') + 1);
    std::replace(label_.begin(), label_.end(), '_', ' ');
    std::transform(label_.begin(), label_.end(), label_.begin(),
        [](unsigned char character) { return static_cast<char>(std::toupper(character)); });
    const auto filter = entry_.path.find(".filter.");
    if (filter != std::string::npos)
    {
        const auto start = filter + std::string(".filter.").size();
        label_ = entry_.path.substr(start, entry_.path.find('.', start) - start) + " " + label_;
    }
}

bool StateValueViewModel::Apply(shared::clients::ParameterEntry entry)
{
    if (entry.path != entry_.path)
    {
        throw std::invalid_argument("Cannot change a state value binding path.");
    }
    if (entry.value == entry_.value && entry.minimum == entry_.minimum && entry.maximum == entry_.maximum &&
        entry.effectiveMinimum == entry_.effectiveMinimum && entry.effectiveMaximum == entry_.effectiveMaximum)
    {
        return false;
    }
    entry_ = std::move(entry);
    return true;
}

const shared::clients::ParameterEntry& StateValueViewModel::Entry() const { return entry_; }
const std::string& StateValueViewModel::Path() const { return entry_.path; }
const std::string& StateValueViewModel::Label() const { return label_; }
void StateValueViewModel::SetEnabled(bool enabled) { enabled_ = enabled; }
bool StateValueViewModel::Enabled() const { return enabled_ && available_; }
void StateValueViewModel::SetAvailable(bool available) { available_ = available; }
bool StateValueViewModel::Available() const { return available_; }

} // namespace consolidator::vst3::shared::presentation
