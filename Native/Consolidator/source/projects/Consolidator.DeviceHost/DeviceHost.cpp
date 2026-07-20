#include "DeviceHost.h"

#include <variant>
#include <type_traits>
#include <utility>

namespace consolidator::host {

DeviceHost::DeviceHost(EventHandler eventHandler)
    : eqStore([this](domain::StoreRevision revision, domain::RequestId requestId) {
        Publish(domain::StoreUpdatedEvent{ "eq", revision, requestId });
    }),
      analyzerWorkflow([this](domain::Event event) { Publish(std::move(event)); }),
      fitWorkflow(eqStore, [this](domain::Event event) { Publish(std::move(event)); }),
      eventHandler(std::move(eventHandler)) {}

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
            else if constexpr (std::is_same_v<Command, domain::ListenAnalyzerCommand>) analyzerWorkflow.Handle(value);
            else if constexpr (std::is_same_v<Command, domain::StartFitCommand> ||
                               std::is_same_v<Command, domain::CancelFitCommand> ||
                               std::is_same_v<Command, domain::ClearFitCommand> ||
                               std::is_same_v<Command, domain::CompleteFitCommand> ||
                               std::is_same_v<Command, domain::FailFitCommand>) fitWorkflow.Handle(value);
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

} // namespace consolidator::host
