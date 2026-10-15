#pragma once

#include <functional>
#include <string>
#include <vector>

#include "Features/Equalizer/Bindings/AnalyzerIntents.h"
#include "Features/Equalizer/Bindings/ParameterEdit.h"
#include "Shared/Clients/ParameterEntry.h"
#include "Shared/Clients/TargetStateClient.h"
#include "Shared/Editing/DialGestureSession.h"
#include "Shared/Protocol/ProtocolFrame.h"
#include "Features/Equalizer/ViewModels/AnalyzerViewModel.h"

namespace consolidator::vst3::features::equalizer::bindings
{

struct AnalyzerBindingActions
{
    std::function<bool(const std::string&, std::vector<shared::protocol::StateWriteEntry>)> begin;
    std::function<void()> ended;
    std::function<void(const std::string&)> reset;
    std::function<void(const std::string&, bool)> bypass;
};

class AnalyzerBindings final
{
public:
    AnalyzerBindings(const shared::clients::TargetStateClient& state, shared::editing::DialGestureSession& gesture,
        AnalyzerBindingActions actions);
    [[nodiscard]] AnalyzerIntents Intents();
    [[nodiscard]] features::equalizer::viewmodels::AnalyzerViewModel& Model();
    void SetSnapshot(const std::string& context, std::vector<shared::clients::ParameterEntry> entries, double sampleRate);
    void Apply(const shared::clients::ParameterEntry& entry);
    void SetScope(bool groupScope);
    void ApplyPreview(const std::vector<shared::protocol::StateWriteEntry>& writes);

private:
    [[nodiscard]] std::vector<shared::protocol::StateWriteEntry> Encode(std::vector<ParameterEdit> edits) const;

    const shared::clients::TargetStateClient& state_;
    shared::editing::DialGestureSession& gesture_;
    AnalyzerBindingActions actions_;
    features::equalizer::viewmodels::AnalyzerViewModel model_;
    bool groupScope_{};
};

} // namespace consolidator::vst3::features::equalizer::bindings
