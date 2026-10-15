#include "Features/Tracks/ViewModels/TracksViewModel.h"

#include <algorithm>
#include <cmath>

namespace consolidator::vst3::features::tracks::viewmodels
{

TracksViewModel::TracksViewModel(const shared::clients::RegistryClient& registry,
    const shared::clients::TrackLevelClient& levels, std::uint64_t localInstance)
    : registry_(registry), levels_(levels), localInstance_(localInstance), focusedInstance_(localInstance)
{
}

void TracksViewModel::Refresh()
{
    ConfirmFeedback();
    std::erase_if(selected_, [this](const auto& selection)
    {
        const auto bank = registry_.FindBank(selection.first, selection.second);
        return bank == nullptr || bank->group.has_value();
    });
}

TrackFeedback& TracksViewModel::Feedback() { return feedback_; }

void TracksViewModel::ConfirmFeedback()
{
    for (const auto& row : Rows())
    {
        for (const auto& processor : row.processors)
        {
            feedback_.ConfirmBypass(row.id, processor.id, processor.bypass);
        }
        for (const auto& bank : row.banks)
        {
            feedback_.ConfirmBypass(row.id, "bank." + std::to_string(bank.index), bank.bypass);
        }
    }
}

void TracksViewModel::SetFocus(std::uint64_t instance, int bank, bool groupScope)
{
    focusedInstance_ = instance;
    focusedBank_ = bank;
    groupScope_ = groupScope;
}

void TracksViewModel::ClearSelection() { selected_.clear(); }

void TracksViewModel::Select(std::uint64_t instance, int index)
{
    const auto bank = FindBank(instance, index);
    if (bank == nullptr)
    {
        return;
    }
    selected_.clear();
    if (!bank->group)
    {
        selected_[instance] = index;
    }
}

bool TracksViewModel::ToggleSelection(std::uint64_t instance, int index)
{
    const auto bank = FindBank(instance, index);
    if (bank == nullptr || bank->group)
    {
        return false;
    }
    const auto found = selected_.find(instance);
    if (found != selected_.end() && found->second == index)
    {
        selected_.erase(found);
    }
    else if (found == selected_.end())
    {
        selected_[instance] = index;
    }
    else
    {
        return false;
    }
    return true;
}

const std::map<std::uint64_t, int>& TracksViewModel::SelectedBanks() const { return selected_; }
const std::vector<shared::clients::RegistryInstance>& TracksViewModel::Rows() const
{
    return registry_.Instances();
}
const shared::clients::RegistryBank* TracksViewModel::FindBank(std::uint64_t instance, int bank) const
{
    return registry_.FindBank(instance, bank);
}

BankCellPresentation TracksViewModel::BankCell(std::uint64_t instance,
    const shared::clients::RegistryBank& bank) const
{
    const bool selected = selected_.contains(instance) && selected_.at(instance) == bank.index;
    const bool focused = instance == focusedInstance_ && bank.index == focusedBank_;
    const auto group = FocusedGroup();
    return { selected, focused, selected || focused || (groupScope_ && group && bank.group == group) };
}

bool TracksViewModel::Local(std::uint64_t instance) const { return instance == localInstance_; }
bool TracksViewModel::InstanceFocused(std::uint64_t instance) const
{
    return instance == focusedInstance_;
}
int TracksViewModel::FocusedBank() const { return focusedBank_; }
std::uint64_t TracksViewModel::FocusedInstance() const { return focusedInstance_; }
bool TracksViewModel::GroupScope() const { return groupScope_; }

TrackActions TracksViewModel::Actions() const
{
    const auto bank = FindBank(focusedInstance_, focusedBank_);
    const bool clear = std::any_of(Rows().begin(), Rows().end(), [](const auto& instance)
    {
        return std::any_of(instance.banks.begin(), instance.banks.end(),
            [](const auto& candidate) { return candidate.group && *candidate.group != 0; });
    });
    return { registry_.Ready() && selected_.size() >= 2 && registry_.CanCreateGroup(),
        bank != nullptr && bank->group && *bank->group != 0, clear,
        bank != nullptr && bank->active, bank != nullptr && bank->group.has_value() };
}

std::optional<std::uint32_t> TracksViewModel::FocusedGroup() const
{
    const auto bank = FindBank(focusedInstance_, focusedBank_);
    return bank != nullptr ? bank->group : std::optional<std::uint32_t>{};
}

double TracksViewModel::OutputAlpha(double outputLevelDb)
{
    return std::pow(std::clamp((outputLevelDb + 60.0) / 60.0, 0.0, 1.0), 1.8);
}

double TracksViewModel::OutputAlphaFor(std::uint64_t instance) const
{
    return OutputAlpha(levels_.Level(instance));
}

std::string TracksViewModel::GroupLabel(std::uint32_t group)
{
    std::string label;
    auto value = static_cast<std::uint64_t>(group) + 1;
    while (value != 0)
    {
        --value;
        label.insert(label.begin(), static_cast<char>('A' + value % 26));
        value /= 26;
    }
    return label;
}

std::string TracksViewModel::Label(const shared::clients::RegistryInstance& instance)
{
    return instance.label.empty() ? "Instance " + std::to_string(instance.id) : instance.label;
}

} // namespace consolidator::vst3::features::tracks::viewmodels
