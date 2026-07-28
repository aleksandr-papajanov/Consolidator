#include "AnalyzerWorkflow.h"

#include <utility>

namespace consolidator::host {

AnalyzerWorkflow::AnalyzerWorkflow(EventHandler eventHandler)
    : eventHandler(std::move(eventHandler)) {}

void AnalyzerWorkflow::Handle(const domain::ClearAnalyzerCommand&) {
    ++state.sessionId.value;
    Publish(domain::OperationChangedEvent{
        "analyzer.clear",
        state.sessionId,
        domain::OperationStatus::Completed,
        0.0,
        {}
    });
}

void AnalyzerWorkflow::Handle(const domain::SetAnalyzerViewCommand& command) {
    const auto changed = state.viewVisible != command.visible || state.viewMode != command.mode;
    state.viewVisible = command.visible;
    state.viewMode = command.mode;
    if (changed) {
        Publish(domain::AnalyzerViewChangedEvent{ state.viewVisible, state.viewMode });
    }
}

const domain::AnalyzerState& AnalyzerWorkflow::State() const noexcept {
    return state;
}

void AnalyzerWorkflow::Publish(domain::Event event) const {
    if (eventHandler) eventHandler(std::move(event));
}

} // namespace consolidator::host
