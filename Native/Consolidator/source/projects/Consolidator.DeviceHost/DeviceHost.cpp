#include "DeviceHost.h"

#include <cmath>
#include <variant>
#include <type_traits>
#include <utility>

namespace consolidator::host {

DeviceHost::DeviceHost(EventHandler eventHandler)
    : eqStore([this](domain::StoreRevision revision, domain::RequestId requestId) {
        Publish(domain::StoreUpdatedEvent{ "eq", revision, requestId });
    }), eventHandler(std::move(eventHandler)) {}

void DeviceHost::Handle(const domain::Command& command) {
    std::vector<domain::Event> events;
    {
        std::lock_guard<std::mutex> lock(mutex);
        struct EventBatchGuard final {
            std::vector<domain::Event>*& active;
            explicit EventBatchGuard(
                std::vector<domain::Event>*& active,
                std::vector<domain::Event>& events
            ) : active(active) { active = &events; }
            ~EventBatchGuard() { active = nullptr; }
        } eventBatch{ activeEvents, events };
        std::visit([this](const auto& value) {
            using Command = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<Command, domain::AttachComponentCommand> ||
                          std::is_same_v<Command, domain::DetachComponentCommand>) {
                HandleComponent(value);
            }
            else if constexpr (std::is_same_v<Command, domain::SetEqParameterCommand>) PublishResult(eqStore.SetParameter(value), value.requestId);
            else if constexpr (std::is_same_v<Command, domain::SetEqBypassCommand>) PublishResult(eqStore.SetBypass(value), value.requestId);
            else if constexpr (std::is_same_v<Command, domain::ResetEqFilterCommand>) PublishResult(eqStore.ResetFilter(value), value.requestId);
            else if constexpr (std::is_same_v<Command, domain::AddEqBankCommand>) PublishResult(eqStore.AddBank(value), value.requestId);
            else if constexpr (std::is_same_v<Command, domain::RemoveEqBankCommand>) PublishResult(eqStore.RemoveBank(value), value.requestId);
            else if constexpr (std::is_same_v<Command, domain::RenameEqBankCommand>) PublishResult(eqStore.RenameBank(value), value.requestId);
            else if constexpr (std::is_same_v<Command, domain::SelectEqBankCommand>) PublishResult(eqStore.SelectBank(value), value.requestId);
            else if constexpr (std::is_same_v<Command, domain::ListenAnalyzerCommand>) HandleAnalyzer(value);
            else if constexpr (std::is_same_v<Command, domain::StartFitCommand> ||
                               std::is_same_v<Command, domain::CancelFitCommand> ||
                               std::is_same_v<Command, domain::ClearFitCommand> ||
                               std::is_same_v<Command, domain::CompleteFitCommand> ||
                               std::is_same_v<Command, domain::FailFitCommand>) HandleFit(value);
        }, command);
    }
    Dispatch(events);
}

const EqStore& DeviceHost::Eq() const noexcept {
    return eqStore;
}

domain::StoreRevision DeviceHost::Revision() const noexcept {
    return eqStore.Revision();
}

bool DeviceHost::RestoreEq(domain::EqState state, domain::StoreRevision revision) {
    std::lock_guard<std::mutex> lock(mutex);
    const auto result = eqStore.Replace(std::move(state), revision);
    return result.Accepted();
}

void DeviceHost::HandleComponent(const domain::AttachComponentCommand& command) {
    const auto existing = components.find(command.componentId.value);
    if (existing != components.end() && existing->second == command.type) {
        Publish(domain::ComponentAttachedEvent{ command.componentId, command.type });
        return;
    }
    if (existing != components.end()) {
        Publish(domain::CommandRejectedEvent{ command.requestId, "component_id_conflict" });
        return;
    }
    components[command.componentId.value] = command.type;
    Publish(domain::ComponentAttachedEvent{ command.componentId, command.type });
}

void DeviceHost::HandleComponent(const domain::DetachComponentCommand& command) {
    if (components.erase(command.componentId.value) == 0) {
        Publish(domain::CommandRejectedEvent{ command.requestId, "component_not_attached" });
    }
}

void DeviceHost::Publish(domain::Event event) {
    if (activeEvents) activeEvents->push_back(std::move(event));
    else if (eventHandler) eventHandler(event);
}

void DeviceHost::Dispatch(const std::vector<domain::Event>& events) const {
    if (!eventHandler) return;
    for (const auto& event : events) eventHandler(event);
}

void DeviceHost::PublishResult(const UpdateResult& result, domain::RequestId requestId) {
    if (result.status == UpdateStatus::Rejected) {
        Publish(domain::CommandRejectedEvent{ requestId, result.error });
    }
}

void DeviceHost::HandleAnalyzer(const domain::ListenAnalyzerCommand& command) {
    const auto nextStatus = command.enabled
        ? domain::AnalyzerState::Status::Listening
        : domain::AnalyzerState::Status::Idle;
    if (analyzerState.status == nextStatus) return;
    analyzerState.status = nextStatus;
    if (command.enabled) ++analyzerState.sessionId.value;
    Publish(domain::OperationChangedEvent{
        "analyzer", analyzerState.sessionId,
        command.enabled ? domain::OperationStatus::Capturing : domain::OperationStatus::Idle,
        0.0, {}
    });
}

void DeviceHost::HandleFit(const domain::StartFitCommand& command) {
    if (approximatorState.status == domain::ApproximatorState::Status::Processing) {
        Publish(domain::CommandRejectedEvent{ command.requestId, "fit_in_progress" });
        return;
    }
    approximatorState.status = domain::ApproximatorState::Status::Processing;
    ++approximatorState.sessionId.value;
    activeFitBankId = { eqStore.State().selectedBankId };
    Publish(domain::OperationChangedEvent{
        "fit", approximatorState.sessionId, domain::OperationStatus::Starting, 0.0, {}
    });
}

void DeviceHost::HandleFit(const domain::CancelFitCommand& command) {
    if (approximatorState.status != domain::ApproximatorState::Status::Processing ||
        approximatorState.sessionId != command.sessionId) {
        Publish(domain::CommandRejectedEvent{ command.requestId, "stale_fit_session" });
        return;
    }
    approximatorState.status = domain::ApproximatorState::Status::Idle;
    activeFitBankId = {};
    Publish(domain::OperationChangedEvent{
        "fit", command.sessionId, domain::OperationStatus::Cancelled, 0.0, {}
    });
}

void DeviceHost::HandleFit(const domain::ClearFitCommand& command) {
    (void)command;
    approximatorState.status = domain::ApproximatorState::Status::Idle;
    approximatorState.progress = 0.0;
    approximatorState.loss = 0.0;
    approximatorState.error.clear();
    activeFitBankId = {};
    Publish(domain::OperationChangedEvent{
        "fit", approximatorState.sessionId, domain::OperationStatus::Idle, 0.0, {}
    });
}

void DeviceHost::HandleFit(const domain::CompleteFitCommand& command) {
    if (approximatorState.status != domain::ApproximatorState::Status::Processing ||
        command.result.sessionId != approximatorState.sessionId ||
        command.result.bankId != activeFitBankId) {
        Publish(domain::CommandRejectedEvent{ command.requestId, "stale_fit_result" });
        return;
    }
    if (!std::isfinite(command.result.loss)) {
        approximatorState.status = domain::ApproximatorState::Status::Failed;
        approximatorState.error = "invalid_fit_result";
        activeFitBankId = {};
        Publish(domain::CommandRejectedEvent{ command.requestId, "invalid_fit_result" });
        Publish(domain::OperationChangedEvent{
            "fit", approximatorState.sessionId, domain::OperationStatus::Failed,
            1.0, "invalid_fit_result"
        });
        return;
    }
    const auto result = eqStore.ApplyFitResult(command);
    if (!result.Accepted()) {
        approximatorState.status = domain::ApproximatorState::Status::Failed;
        approximatorState.error = result.error;
        activeFitBankId = {};
        Publish(domain::CommandRejectedEvent{ command.requestId, result.error });
        Publish(domain::OperationChangedEvent{
            "fit", approximatorState.sessionId, domain::OperationStatus::Failed,
            1.0, result.error
        });
        return;
    }
    approximatorState.status = domain::ApproximatorState::Status::Completed;
    approximatorState.progress = 1.0;
    approximatorState.loss = command.result.loss;
    approximatorState.error.clear();
    activeFitBankId = {};
    Publish(domain::OperationChangedEvent{
        "fit", approximatorState.sessionId, domain::OperationStatus::Completed,
        1.0, {}
    });
}

void DeviceHost::HandleFit(const domain::FailFitCommand& command) {
    if (approximatorState.status != domain::ApproximatorState::Status::Processing ||
        command.sessionId != approximatorState.sessionId) {
        Publish(domain::CommandRejectedEvent{ command.requestId, "stale_fit_failure" });
        return;
    }
    approximatorState.status = domain::ApproximatorState::Status::Failed;
    approximatorState.error = command.error;
    activeFitBankId = {};
    Publish(domain::OperationChangedEvent{
        "fit", approximatorState.sessionId, domain::OperationStatus::Failed,
        approximatorState.progress, command.error
    });
}

} // namespace consolidator::host
