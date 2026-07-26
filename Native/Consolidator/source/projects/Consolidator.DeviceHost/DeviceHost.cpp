#include "DeviceHost.h"

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
      fitWorkflow(
          eqStore, inputGainStore, compressorStore, saturatorStore, outputGainStore,
          [this](domain::Event event) { Publish(std::move(event)); }),
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
            if constexpr (std::is_same_v<Command, domain::SetEqParameterCommand>) PublishResult(eqStore.SetParameter(value), value.requestId);
            else if constexpr (std::is_same_v<Command, domain::SetEqParameterIndexCommand>) PublishResult(eqStore.SetParameterAtIndex(value), value.requestId);
            else if constexpr (std::is_same_v<Command, domain::SetEqBypassCommand>) PublishResult(eqStore.SetBypass(value), value.requestId);
            else if constexpr (std::is_same_v<Command, domain::ResetEqFilterCommand>) PublishResult(eqStore.ResetFilter(value), value.requestId);
            else if constexpr (std::is_same_v<Command, domain::SetEqChainBypassCommand>) PublishResult(eqStore.SetChainBypass(value), value.requestId);
            else if constexpr (std::is_same_v<Command, domain::SetEqChainSoloCommand>) PublishResult(eqStore.SetChainSolo(value), value.requestId);
            else if constexpr (std::is_same_v<Command, domain::ResetEqChainCommand>) PublishResult(eqStore.ResetChain(value), value.requestId);
            else if constexpr (std::is_same_v<Command, domain::JoinEqBanksCommand>) PublishResult(eqStore.JoinBanks(value), value.requestId);
            else if constexpr (std::is_same_v<Command, domain::CommitHiddenEqBankCommand>) fitWorkflow.Handle(value);
            else if constexpr (std::is_same_v<Command, domain::SetEqBankLinkCommand>) PublishResult(eqStore.SetBankLink(value), value.requestId);
            else if constexpr (std::is_same_v<Command, domain::SelectEqBankCommand>) PublishResult(eqStore.SelectBank(value), value.requestId);
            else if constexpr (std::is_same_v<Command, domain::SetGainParameterCommand>) {
                auto& store = value.stage == domain::GainStage::Input ? inputGainStore : outputGainStore;
                PublishResult(store.SetParameter(value), value.requestId);
            }
            else if constexpr (std::is_same_v<Command, domain::SetProcessorLinkCommand>) {
                if (value.device == "input_gain") PublishResult(inputGainStore.SetLink(value), value.requestId);
                else if (value.device == "compressor") PublishResult(compressorStore.SetLink(value), value.requestId);
                else if (value.device == "saturator") PublishResult(saturatorStore.SetLink(value), value.requestId);
                else if (value.device == "output_gain") PublishResult(outputGainStore.SetLink(value), value.requestId);
            }
            else if constexpr (std::is_same_v<Command, domain::SetCompressorParameterCommand>) PublishResult(compressorStore.SetParameter(value), value.requestId);
            else if constexpr (std::is_same_v<Command, domain::SetCompressorBypassCommand>) PublishResult(compressorStore.SetBypass(value), value.requestId);
            else if constexpr (std::is_same_v<Command, domain::SetCompressorModeCommand>) PublishResult(compressorStore.SetMode(value), value.requestId);
            else if constexpr (std::is_same_v<Command, domain::SetCompressorDetectorParameterCommand>) PublishResult(compressorStore.SetDetectorParameter(value), value.requestId);
            else if constexpr (std::is_same_v<Command, domain::SetCompressorDetectorListenCommand>) PublishResult(compressorStore.SetDetectorListen(value), value.requestId);
            else if constexpr (std::is_same_v<Command, domain::ResetCompressorCommand>) PublishResult(compressorStore.Reset(value), value.requestId);
            else if constexpr (std::is_same_v<Command, domain::SetSaturatorParameterCommand>) PublishResult(saturatorStore.SetParameter(value), value.requestId);
            else if constexpr (std::is_same_v<Command, domain::SetSaturatorBypassCommand>) PublishResult(saturatorStore.SetBypass(value), value.requestId);
            else if constexpr (std::is_same_v<Command, domain::SetSaturatorModeCommand>) PublishResult(saturatorStore.SetMode(value), value.requestId);
            else if constexpr (std::is_same_v<Command, domain::SetSaturatorDetectorParameterCommand>) PublishResult(saturatorStore.SetDetectorParameter(value), value.requestId);
            else if constexpr (std::is_same_v<Command, domain::SetSaturatorDetectorListenCommand>) PublishResult(saturatorStore.SetDetectorListen(value), value.requestId);
            else if constexpr (std::is_same_v<Command, domain::ResetSaturatorCommand>) PublishResult(saturatorStore.Reset(value), value.requestId);
            else if constexpr (std::is_same_v<Command, domain::ListenAnalyzerCommand>) analyzerWorkflow.Handle(value);
            else if constexpr (std::is_same_v<Command, domain::SetAnalyzerViewCommand>) analyzerWorkflow.Handle(value);
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
    if (instanceId.empty()) return false;
    std::lock_guard<std::mutex> lock(mutex);
    const auto eqResult = eqStore.Replace(std::move(eq), revision);
    const auto inputGainResult = inputGainStore.Replace(processor.inputGain, 0);
    const auto compressorResult = compressorStore.Replace(processor.compressor, 0);
    const auto saturatorResult = saturatorStore.Replace(processor.saturator, 0);
    const auto outputGainResult = outputGainStore.Replace(processor.outputGain, 0);
    const auto restored = eqResult.Accepted() && inputGainResult.Accepted() && compressorResult.Accepted() &&
        saturatorResult.Accepted() && outputGainResult.Accepted();
    if (restored) this->instanceId = std::move(instanceId);
    return restored;
}

const std::string& DeviceHost::InstanceId() const noexcept { return instanceId; }

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
