#include "Features/Equalizer/Bindings/AnalyzerBindings.h"

#include <utility>

namespace consolidator::vst3::features::equalizer::bindings
{

AnalyzerBindings::AnalyzerBindings(const shared::clients::TargetStateClient& state, shared::editing::DialGestureSession& gesture,
    AnalyzerBindingActions actions)
    : state_(state), gesture_(gesture), actions_(std::move(actions))
{
}

void AnalyzerBindings::SetScope(bool groupScope) { groupScope_ = groupScope; }
features::equalizer::viewmodels::AnalyzerViewModel& AnalyzerBindings::Model() { return model_; }
void AnalyzerBindings::SetSnapshot(const std::string& context,
    std::vector<shared::clients::ParameterEntry> entries, double sampleRate)
{
    model_.SetSnapshot(context, std::move(entries), sampleRate, state_.Target().bank);
}
void AnalyzerBindings::Apply(const shared::clients::ParameterEntry& entry) { model_.Apply(entry); }

std::vector<shared::protocol::StateWriteEntry> AnalyzerBindings::Encode(std::vector<ParameterEdit> edits) const
{
    std::vector<shared::protocol::StateWriteEntry> writes;
    writes.reserve(edits.size());
    for (auto& edit : edits)
    {
        const auto mode = state_.WriteMode(edit.path, groupScope_);
        writes.push_back({ std::move(edit.path), std::move(edit.value), mode });
    }
    return writes;
}

AnalyzerIntents AnalyzerBindings::Intents()
{
    return {
        [this](const std::string& path, std::vector<ParameterEdit> edits)
            { return actions_.begin(path, Encode(std::move(edits))); },
        [this](std::vector<ParameterEdit> edits) { gesture_.SetValues(Encode(std::move(edits))); },
        [this](std::vector<ParameterEdit> edits)
        {
            gesture_.End(Encode(std::move(edits)));
            actions_.ended();
        },
        actions_.reset,
        actions_.bypass
    };
}

void AnalyzerBindings::ApplyPreview(const std::vector<shared::protocol::StateWriteEntry>& writes)
{
    std::vector<ParameterEdit> edits;
    edits.reserve(writes.size());
    for (const auto& write : writes)
    {
        edits.push_back({ write.path, write.value });
    }
    model_.ApplyPreview(std::move(edits));
}

} // namespace consolidator::vst3::features::equalizer::bindings
