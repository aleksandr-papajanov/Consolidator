#include "AnalyzerWorkflow.h"

#include <utility>

namespace consolidator::host {

AnalyzerWorkflow::AnalyzerWorkflow(EventHandler eventHandler)
    : eventHandler(std::move(eventHandler)) {}

void AnalyzerWorkflow::Handle(const domain::ListenAnalyzerCommand& command) {
    const auto nextStatus = command.enabled
        ? domain::AnalyzerState::Status::Listening
        : domain::AnalyzerState::Status::Idle;
    if (state.status == nextStatus) return;

    state.status = nextStatus;
    if (command.enabled) ++state.sessionId.value;
    Publish(domain::OperationChangedEvent{
        "analyzer",
        state.sessionId,
        command.enabled ? domain::OperationStatus::Capturing : domain::OperationStatus::Idle,
        0.0,
        {}
    });
}

const domain::AnalyzerState& AnalyzerWorkflow::State() const noexcept {
    return state;
}

void AnalyzerWorkflow::Publish(domain::Event event) const {
    if (eventHandler) eventHandler(std::move(event));
}

} // namespace consolidator::host
