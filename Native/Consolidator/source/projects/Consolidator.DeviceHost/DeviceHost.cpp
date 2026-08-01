#include "DeviceHost.h"

#include <algorithm>
#include <optional>
#include <variant>
#include <type_traits>
#include <utility>

namespace consolidator::host {

DeviceHost::DeviceHost(EventHandler eventHandler)
    : eqStore([this](domain::StoreRevision, domain::RequestId requestId) {
        Publish(domain::StoreUpdatedEvent{ "eq", Revision(), requestId });
    }),
      inputGainStore([this](domain::StoreRevision, domain::RequestId requestId) {
        Publish(domain::StoreUpdatedEvent{ "input_gain", Revision(), requestId });
      }),
      compressorStore([this](domain::StoreRevision, domain::RequestId requestId) {
        Publish(domain::StoreUpdatedEvent{ "compressor", Revision(), requestId });
      }),
      saturatorStore([this](domain::StoreRevision, domain::RequestId requestId) {
        Publish(domain::StoreUpdatedEvent{ "saturator", Revision(), requestId });
      }),
      outputGainStore([this](domain::StoreRevision, domain::RequestId requestId) {
        Publish(domain::StoreUpdatedEvent{ "output_gain", Revision(), requestId });
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
        const auto undoable = IsUndoableCommand(command);
        const auto isHistoryNavigation =
            std::holds_alternative<domain::UndoHistoryCommand>(command) ||
            std::holds_alternative<domain::RedoHistoryCommand>(command);
        const auto revisionBefore = Revision();
        const auto couldUndo = history.CanUndo();
        const auto couldRedo = history.CanRedo();
        auto stateBefore = undoable && !history.InTransaction()
            ? std::optional{ CaptureHistoryState() }
            : std::optional<DeviceHistoryState>{};
        std::visit([this](const auto& value) {
            using Command = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<Command, domain::SetEqParameterCommand>) {
                PublishResult(eqStore.SetParameter(value), value.requestId);
            }
            else if constexpr (std::is_same_v<Command, domain::SetEqParameterIndexCommand>) {
                PublishResult(eqStore.SetParameterAtIndex(value), value.requestId);
            }
            else if constexpr (std::is_same_v<Command, domain::SetEqBypassCommand>) PublishResult(eqStore.SetBypass(value), value.requestId);
            else if constexpr (std::is_same_v<Command, domain::ResetEqFilterCommand>) PublishResult(eqStore.ResetFilter(value), value.requestId);
            else if constexpr (std::is_same_v<Command, domain::SetEqChainBypassCommand>) PublishResult(eqStore.SetChainBypass(value), value.requestId);
            else if constexpr (std::is_same_v<Command, domain::SetEqChainSoloCommand>) PublishResult(eqStore.SetChainSolo(value), value.requestId);
            else if constexpr (std::is_same_v<Command, domain::ResetEqChainCommand>) PublishResult(eqStore.ResetChain(value), value.requestId);
            else if constexpr (std::is_same_v<Command, domain::ResetAllEqBanksCommand>) PublishResult(eqStore.ResetAll(value), value.requestId);
            else if constexpr (std::is_same_v<Command, domain::JoinEqBanksCommand>) PublishResult(eqStore.JoinBanks(value), value.requestId);
            else if constexpr (std::is_same_v<Command, domain::CommitHiddenEqBankCommand>) fitWorkflow.Handle(value);
            else if constexpr (std::is_same_v<Command, domain::CommitAllEqBanksCommand>) fitWorkflow.Handle(value);
            else if constexpr (std::is_same_v<Command, domain::SetEqBankLinkCommand>) PublishResult(eqStore.SetBankLink(value), value.requestId);
            else if constexpr (std::is_same_v<Command, domain::SelectEqBankCommand>) PublishResult(eqStore.SelectBank(value), value.requestId);
            else if constexpr (std::is_same_v<Command, domain::SetGainParameterCommand>) {
                auto& store = value.stage == domain::GainStage::Input ? inputGainStore : outputGainStore;
                PublishResult(store.SetParameter(value), value.requestId);
            }
            else if constexpr (std::is_same_v<Command, domain::SetCompressorParameterCommand>) {
                PublishResult(compressorStore.SetParameter(value), value.requestId);
            }
            else if constexpr (std::is_same_v<Command, domain::SetCompressorBypassCommand>) PublishResult(compressorStore.SetBypass(value), value.requestId);
            else if constexpr (std::is_same_v<Command, domain::SetCompressorDetectorParameterCommand>) PublishResult(compressorStore.SetDetectorParameter(value), value.requestId);
            else if constexpr (std::is_same_v<Command, domain::SetCompressorDetectorListenCommand>) PublishResult(compressorStore.SetDetectorListen(value), value.requestId);
            else if constexpr (std::is_same_v<Command, domain::ResetCompressorCommand>) PublishResult(compressorStore.Reset(value), value.requestId);
            else if constexpr (std::is_same_v<Command, domain::SetSaturatorParameterCommand>) {
                PublishResult(saturatorStore.SetParameter(value), value.requestId);
            }
            else if constexpr (std::is_same_v<Command, domain::SetSaturatorBypassCommand>) PublishResult(saturatorStore.SetBypass(value), value.requestId);
            else if constexpr (std::is_same_v<Command, domain::SetSaturatorDetectorParameterCommand>) PublishResult(saturatorStore.SetDetectorParameter(value), value.requestId);
            else if constexpr (std::is_same_v<Command, domain::SetSaturatorDetectorListenCommand>) PublishResult(saturatorStore.SetDetectorListen(value), value.requestId);
            else if constexpr (std::is_same_v<Command, domain::ResetSaturatorCommand>) PublishResult(saturatorStore.Reset(value), value.requestId);
            else if constexpr (std::is_same_v<Command, domain::BeginHistoryCommand>) {
                const auto linkId = ActiveLinkId();
                if (history.Begin(CaptureHistoryState(), Revision(), value.operationId, linkId)) {
                    Publish(domain::HistoryBeganEvent{ value.operationId, linkId });
                }
            }
            else if constexpr (std::is_same_v<Command, domain::EndHistoryCommand>) {
                const auto linkId = ActiveLinkId();
                const auto wasActive = history.IsTransaction(value.operationId);
                history.End(Revision(), value.operationId);
                if (wasActive) Publish(domain::HistoryEndedEvent{ value.operationId, linkId });
            }
            else if constexpr (std::is_same_v<Command, domain::UndoHistoryCommand>) {
                if (const auto restore = history.Undo(CaptureHistoryState())) {
                    if (RestoreHistoryState(std::move(restore->state), value.requestId)) {
                        Publish(domain::HistoryRestoredEvent{ true, restore->operationId, restore->linkId });
                    }
                }
            }
            else if constexpr (std::is_same_v<Command, domain::RedoHistoryCommand>) {
                if (const auto restore = history.Redo(CaptureHistoryState())) {
                    if (RestoreHistoryState(std::move(restore->state), value.requestId)) {
                        Publish(domain::HistoryRestoredEvent{ false, restore->operationId, restore->linkId });
                    }
                }
            }
            else if constexpr (std::is_same_v<Command, domain::RestoreHistoryOperationCommand>) {
                if (const auto restore = history.Restore(value.operationId, value.isUndo, CaptureHistoryState())) {
                    RestoreHistoryState(std::move(restore->state), value.requestId);
                }
            }
            else if constexpr (std::is_same_v<Command, domain::ClearAnalyzerCommand>) analyzerWorkflow.Handle(value);
            else if constexpr (std::is_same_v<Command, domain::SetAnalyzerViewCommand>) analyzerWorkflow.Handle(value);
            else if constexpr (std::is_same_v<Command, domain::StartFitCommand> ||
                               std::is_same_v<Command, domain::CancelFitCommand> ||
                               std::is_same_v<Command, domain::ClearFitCommand> ||
                               std::is_same_v<Command, domain::CompleteFitCommand> ||
                               std::is_same_v<Command, domain::FailFitCommand>) fitWorkflow.Handle(value);
        }, command);
        if (stateBefore && Revision() != revisionBefore && !history.InTransaction()) {
            history.Record(std::move(*stateBefore));
        }
        if (isHistoryNavigation || history.CanUndo() != couldUndo || history.CanRedo() != couldRedo) {
            PublishHistoryState();
        }
    }
    Dispatch(events);
}

const EqStore& DeviceHost::Eq() const noexcept {
    return eqStore;
}

const GainStore& DeviceHost::InputGain() const noexcept { return inputGainStore; }
const GainStore& DeviceHost::OutputGain() const noexcept { return outputGainStore; }

const CompressorStore& DeviceHost::Compressor() const noexcept { return compressorStore; }
const SaturatorStore& DeviceHost::Saturator() const noexcept { return saturatorStore; }

domain::StoreRevision DeviceHost::Revision() const noexcept {
    return eqStore.Revision() + inputGainStore.Revision() + compressorStore.Revision() +
        saturatorStore.Revision() + outputGainStore.Revision();
}

bool DeviceHost::Restore(
    domain::EqState eq,
    domain::ProcessorState processor,
    domain::StoreRevision revision,
    std::string instanceId
) {
    if (instanceId.empty()) {
        lastRestoreError = "missing_instance_id";
        return false;
    }
    std::lock_guard<std::mutex> lock(mutex);
    const auto restoreRevision = std::max(revision, Revision() + 1);
    const auto eqResult = eqStore.Replace(std::move(eq), restoreRevision);
    const auto inputGainResult = inputGainStore.Replace(processor.inputGain, restoreRevision);
    const auto compressorResult = compressorStore.Replace(processor.compressor, restoreRevision);
    const auto saturatorResult = saturatorStore.Replace(processor.saturator, restoreRevision);
    const auto outputGainResult = outputGainStore.Replace(processor.outputGain, restoreRevision);
    if (!eqResult.Accepted()) {
        lastRestoreError = eqResult.error;
        return false;
    }
    if (!inputGainResult.Accepted()) {
        lastRestoreError = inputGainResult.error;
        return false;
    }
    if (!compressorResult.Accepted()) {
        lastRestoreError = compressorResult.error;
        return false;
    }
    if (!saturatorResult.Accepted()) {
        lastRestoreError = saturatorResult.error;
        return false;
    }
    if (!outputGainResult.Accepted()) {
        lastRestoreError = outputGainResult.error;
        return false;
    }
    this->instanceId = std::move(instanceId);
    history.Clear();
    lastRestoreError.clear();
    return true;
}

const std::string& DeviceHost::InstanceId() const noexcept { return instanceId; }
const std::string& DeviceHost::LastRestoreError() const noexcept { return lastRestoreError; }

DeviceHistoryState DeviceHost::CaptureHistoryState() const {
    return {
        eqStore.State(),
        { inputGainStore.State(), compressorStore.State(), saturatorStore.State(), outputGainStore.State() }
    };
}

std::string DeviceHost::ActiveLinkId() const {
    const auto& state = eqStore.State();
    const auto bank = state.FindBank(state.selectedBankId);
    return bank ? bank->linkId : std::string{};
}

bool DeviceHost::RestoreHistoryState(DeviceHistoryState state, domain::RequestId requestId) {
    const auto revision = Revision() + 1;
    const auto eqResult = eqStore.Replace(std::move(state.eq), revision);
    const auto inputResult = inputGainStore.Replace(std::move(state.processor.inputGain), revision);
    const auto compressorResult = compressorStore.Replace(std::move(state.processor.compressor), revision);
    const auto saturatorResult = saturatorStore.Replace(std::move(state.processor.saturator), revision);
    const auto outputResult = outputGainStore.Replace(std::move(state.processor.outputGain), revision);
    if (!eqResult.Accepted() || !inputResult.Accepted() || !compressorResult.Accepted() ||
        !saturatorResult.Accepted() || !outputResult.Accepted()) {
        Publish(domain::CommandRejectedEvent{ requestId, "history_restore_failed" });
        return false;
    }
    const auto currentRevision = Revision();
    Publish(domain::StoreUpdatedEvent{ "eq", currentRevision, requestId });
    Publish(domain::StoreUpdatedEvent{ "input_gain", currentRevision, requestId });
    Publish(domain::StoreUpdatedEvent{ "compressor", currentRevision, requestId });
    Publish(domain::StoreUpdatedEvent{ "saturator", currentRevision, requestId });
    Publish(domain::StoreUpdatedEvent{ "output_gain", currentRevision, requestId });
    return true;
}

bool DeviceHost::IsUndoableCommand(const domain::Command& command) {
    return std::visit([](const auto& value) {
        using Command = std::decay_t<decltype(value)>;
        return std::is_same_v<Command, domain::SetEqParameterCommand> ||
            std::is_same_v<Command, domain::SetEqBypassCommand> ||
            std::is_same_v<Command, domain::ResetEqFilterCommand> ||
            std::is_same_v<Command, domain::SetEqChainBypassCommand> ||
            std::is_same_v<Command, domain::SetEqChainSoloCommand> ||
            std::is_same_v<Command, domain::ResetEqChainCommand> ||
            std::is_same_v<Command, domain::ResetAllEqBanksCommand> ||
            std::is_same_v<Command, domain::JoinEqBanksCommand> ||
            std::is_same_v<Command, domain::SetEqBankLinkCommand> ||
            std::is_same_v<Command, domain::SetGainParameterCommand> ||
            std::is_same_v<Command, domain::SetCompressorParameterCommand> ||
            std::is_same_v<Command, domain::SetCompressorBypassCommand> ||
            std::is_same_v<Command, domain::SetCompressorDetectorParameterCommand> ||
            std::is_same_v<Command, domain::SetCompressorDetectorListenCommand> ||
            std::is_same_v<Command, domain::ResetCompressorCommand> ||
            std::is_same_v<Command, domain::SetSaturatorParameterCommand> ||
            std::is_same_v<Command, domain::SetSaturatorBypassCommand> ||
            std::is_same_v<Command, domain::SetSaturatorDetectorParameterCommand> ||
            std::is_same_v<Command, domain::SetSaturatorDetectorListenCommand> ||
            std::is_same_v<Command, domain::ResetSaturatorCommand> ||
            std::is_same_v<Command, domain::CompleteFitCommand>;
    }, command);
}

void DeviceHost::PublishHistoryState() {
    Publish(domain::HistoryChangedEvent{ history.CanUndo(), history.CanRedo() });
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
