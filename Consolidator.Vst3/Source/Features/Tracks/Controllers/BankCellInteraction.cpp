#include "Features/Tracks/Controllers/BankCellInteraction.h"

#include <string>
#include <utility>

namespace consolidator::vst3::features::tracks::controllers
{

BankCellInteraction::BankCellInteraction(viewmodels::TracksViewModel& model,
    std::function<void(const bindings::TrackIntent&)> emit)
    : model_(model), emit_(std::move(emit))
{
}

void BankCellInteraction::BeginGesture() { visited_.clear(); }

void BankCellInteraction::Handle(std::uint64_t instance, int index, const juce::MouseEvent& event, bool dragging)
{
    const auto* bank = model_.FindBank(instance, index);
    if (bank == nullptr || (dragging && visited_.contains({ instance, index })))
    {
        return;
    }
    visited_.insert({ instance, index });
    if (!dragging && event.mods.isCtrlDown())
    {
        const auto item = "bank." + std::to_string(index);
        const bool bypass = !model_.Feedback().Bypass(instance, item, bank->bypass);
        if (event.getNumberOfClicks() == 1)
        {
            model_.Feedback().SetBypass(instance, item, bypass);
        }
        emit_({ event.getNumberOfClicks() > 1 ? bindings::TrackIntentKind::ResetBank
            : bindings::TrackIntentKind::SetBankBypass, instance, index, bypass });
    }
    else if (event.mods.isShiftDown())
    {
        if (model_.ToggleSelection(instance, index))
        {
            emit_({ bindings::TrackIntentKind::SelectionChanged });
        }
    }
    else
    {
        model_.Select(instance, index);
        emit_({ bindings::TrackIntentKind::SelectBank, instance, index });
    }
}

} // namespace consolidator::vst3::features::tracks::controllers
