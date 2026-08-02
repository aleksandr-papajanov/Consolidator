#include "c74_min.h"

#include "AtomAdapter.h"
#include "AtomMessage.h"
#include "CommandCodec.h"
#include "DeviceHost.h"
#include "EventCodec.h"
#include "LinkCoordinator.h"
#include "Definitions/Definitions.h"
#include "MaxDictionarySerializer.h"
#include "Models/ParameterDefinitions.h"
#include "PersistenceCodec.h"
#include "SnapshotCodec.h"
#include "Workflows/LatestWorkflowExecutor.h"

#include <exception>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <string_view>
#include <utility>
#include <variant>
#include <type_traits>

using namespace c74::min;
using namespace consolidator;

class ConsolidatorDeviceHost : public object<ConsolidatorDeviceHost> {
private:
    struct RestoreTask final {
        messaging::MessageObject state;
    };

    struct RestoreResult final {
        persistence::PersistedDeviceState state;
    };

public:
    MIN_DESCRIPTION{ "Consolidator device state host." };
    MIN_TAGS{ "state, host, messaging" };
    MIN_AUTHOR{ "Oleksandr Papaianov" };

    inlet<> commandIn{
        this,
        "(message) commands: command 1 <source> <requestId> <name> <fields>; coordinator_identity <runtimeId> <trackName> <trackOrder>; eq.set_parameter, eq.set_bypass, eq.set_chain_bypass <0|1>, eq.set_chain_solo <0|1>, eq.reset <bankId>, eq.reset_all, eq.join_banks <count> <bankIds...>, eq.commit_hidden <bankId>, eq.commit_all, eq.set_link <bankId> <linkId|->, eq.select_bank, gain.*, compressor.*, saturator.*, history.begin <operationId>, history.end <operationId>, history.undo, history.redo, history.restore <operationId> <undo|redo>, analyzer.clear, analyzer.set_view <0|1> <spectrum|analysis>, fit.start <pointCount> <curveDb...>, fit.complete, fit.fail; bang publishes EQ and DSP snapshots after initialization"
    };
    inlet<> persistenceIn{
        this,
        "(message) persistence: restore <dictionary> or persistence_ready"
    };
    inlet<> coordinatorIn{
        this,
        "(message) coordinator_identity <runtimeId> <trackName> <trackOrder>; coordinator_refresh"
    };

    outlet<> eventOut{
        this,
        "(anything) events/snapshots: event 1 host <eventId> <name> <fields>; snapshot 1 host eq ..."
    };
    outlet<> statusOut{
        this,
        "(anything) status: status initializing|ready|error <code>"
    };
    outlet<> debugOut{
        this,
        "(anything) diagnostics: error <code>"
    };
    outlet<> persistenceOut{
        this,
        "(dictionary) persistence state dictionary"
    };
    outlet<> coordinatorOut{
        this,
        "(unused) coordinator directory is emitted through eventOut"
    };

    queue<> stateDelivery{
        this,
        MIN_FUNCTION {
            stateDeliveryScheduled = false;
            if (ready && (eqSnapshotDirty || dspSnapshotDirty ||
                processorSnapshotDirty)) {
                if (eqSnapshotDirty) PublishEqSnapshot();
                if (dspSnapshotDirty) PublishDspSnapshot();
                if (processorSnapshotDirty) PublishProcessorSnapshot();
                eqSnapshotDirty = false;
                dspSnapshotDirty = false;
                processorSnapshotDirty = false;
            }
            return {};
        }
    };

    queue<> restoreDelivery{
        this,
        MIN_FUNCTION {
            DeliverRestore();
            return {};
        }
    };

    timer<timer_options::defer_delivery> persistenceDelivery{
        this,
        MIN_FUNCTION {
            if (ready && persistenceDirty) {
                persistenceDirty = false;
                PublishPersistence();
                PublishContinuousConfirmations();
            }
            return {};
        }
    };

    message<> command{
        this,
        "command",
        "Accept a typed Host command atom list",
        MIN_FUNCTION {
            if (inlet != 0) {
                debugOut.send("error", "invalid_command_inlet");
                return {};
            }
            auto atoms = maxadapter::AtomAdapter::Read(args);
            if (atoms) atoms->insert(atoms->begin(), "command");
            ApplyCommand(atoms);
            return {};
        }
    };

    message<> list{
        this,
        "list",
        "Accept a complete Host command atom list",
        MIN_FUNCTION {
            if (inlet != 0) return {};
            const auto atoms = maxadapter::AtomAdapter::Read(args);
            if (messaging::AtomMessage::HasCategory(atoms, "command")) ApplyCommand(atoms);
            return {};
        }
    };

    message<> bang{
        this,
        "bang",
        "Publish the current Host snapshot",
        MIN_FUNCTION {
            if (!ready) {
                debugOut.send("error", "host_not_ready");
                return {};
            }
            PublishAllSnapshots();
            return {};
        }
    };

    message<> persistenceReady{
        this,
        "persistence_ready",
        "Mark persistence as restored",
        MIN_FUNCTION {
            if (inlet != 1) debugOut.send("error", "invalid_persistence_inlet");
            else if (restorePending) persistenceReadyRequested = true;
            else FinalizeInitialization();
            return {};
        }
    };

    message<> restore{
        this,
        "restore",
        "Restore a persistence dictionary",
        MIN_FUNCTION {
            if (inlet != 1 || args.size() != 1) {
                debugOut.send("error", "invalid_restore");
                return {};
            }
            const auto object = maxadapter::MaxDictionarySerializer::Deserialize<messaging::MessageObject>(args[0]);
            if (!object) {
                debugOut.send("error", "invalid_restore");
                return {};
            }
            restorePending = true;
            restoreExecutor.Submit(0, { std::move(*object) });
            return {};
        }
    };

    message<> coordinatorIdentity{
        this,
        "coordinator_identity",
        "Register this runtime instance with the process-local link coordinator",
        MIN_FUNCTION {
            if (inlet != 0 || args.size() != 3) {
                debugOut.send("error", "invalid_coordinator_identity");
                return {};
            }
            runtimeId = std::string{ args[0] };
            trackName = std::string{ args[1] };
            trackOrder = static_cast<long>(args[2]);
            host::LinkCoordinator::Instance().RegisterCallbacks(runtimeId, {
                [this](const host::LinkedFilterGesture& gesture) {
                    ApplyLinkedFilterGesture(gesture);
                },
                [this](const host::LinkedProcessorGesture& gesture) {
                    ApplyLinkedProcessorGesture(gesture);
                }
            });
            SyncCoordinator();
            PublishCoordinatorDirectory();
            return {};
        }
    };

    message<> coordinatorRefresh{
        this,
        "coordinator_refresh",
        "Publish the current process-local coordinator directory",
        MIN_FUNCTION {
            if (inlet != 0) {
                debugOut.send("error", "invalid_coordinator_inlet");
                return {};
            }
            PublishCoordinatorDirectory();
            return {};
        }
    };

    message<> coordinatorLimits{
        this,
        "coordinator_limits",
        "Refresh native link limits for the selected bank",
        MIN_FUNCTION {
            if (inlet != 0) return {};
            PublishCoordinatorLimits();
            return {};
        }
    };

    message<> coordinatorRemove{
        this,
        "coordinator_remove",
        "Remove this runtime instance from the process-local link coordinator",
        MIN_FUNCTION {
            if (inlet != 0 || args.size() != 1) return {};
            host::LinkCoordinator::Instance().Remove(std::string{ args[0] });
            return {};
        }
    };

    message<> linkFilterLocal{
        this,
        "link_filter_local",
        "Dispatch a local absolute EQ gesture through the native link coordinator",
        MIN_FUNCTION {
            if (inlet != 0 || args.size() != 4) return {};
            const auto bankId = static_cast<long>(args[0]);
            const auto filterId = static_cast<long>(args[1]);
            const auto parameterIndex = static_cast<std::size_t>(static_cast<long>(args[2]));
            const auto targetValue = static_cast<double>(args[3]);
            const auto* bank = host.Eq().State().FindBank(bankId);
            const auto definition = domain::FilterDefinitions().find(filterId);
            const auto* filter = bank ? bank->FindFilter(filterId) : nullptr;
            if (!bank || bank->linkId.empty() || !filter || definition == domain::FilterDefinitions().end() ||
                parameterIndex >= filter->values.size() || parameterIndex >= definition->second.parameters.size() ||
                !std::isfinite(targetValue)) return {};
            const auto& range = definition->second.parameters[parameterIndex].range;
            host::LinkCoordinator::Instance().Dispatch(host::LinkedFilterGesture{
                bank->linkId, runtimeId, filterId, parameterIndex,
                range.Normalize(filter->values[parameterIndex]), range.Normalize(targetValue)
            });
            return {};
        }
    };

    message<> linkProcessorLocal{
        this,
        "link_processor_local",
        "Dispatch a local absolute processor gesture through the native link coordinator",
        MIN_FUNCTION {
            if (inlet != 0 || args.size() != 3) return {};
            const auto device = std::string{ args[0] };
            const auto parameter = std::string{ args[1] };
            const auto targetValue = static_cast<double>(args[2]);
            const auto* bank = host.Eq().State().SelectedBank();
            const auto definition = models::ParameterDefinitions().find(device + "." + parameter);
            const auto current = ReadProcessorValue(device, parameter);
            if (!bank || bank->linkId.empty() || definition == models::ParameterDefinitions().end() ||
                !current || !std::isfinite(targetValue)) return {};
            host::LinkCoordinator::Instance().Dispatch(host::LinkedProcessorGesture{
                bank->linkId, runtimeId, device, parameter,
                definition->second.Normalize(*current), definition->second.Normalize(targetValue)
            });
            return {};
        }
    };

    ConsolidatorDeviceHost()
        : host([this](const domain::Event& event) {
            if (const auto* updated = std::get_if<domain::StoreUpdatedEvent>(&event)) {
                ScheduleStatePublication(updated->storeName);
                return;
            }
            if (const auto* rejected = std::get_if<domain::CommandRejectedEvent>(&event)) {
                debugOut.send("error", rejected->code);
            }
            messaging::EventCodec::Send(event, { nextEventId++ }, [this](const messaging::AtomList& atoms) {
                eventOut.send(maxadapter::AtomAdapter::Write(atoms));
            });
        }),
          restoreExecutor(
              [](const RestoreTask& task, const workflows::WorkflowCancellation& cancellation) {
                  if (cancellation.IsRequested()) return RestoreResult{};
                  return RestoreResult{
                      persistence::PersistenceCodec::Deserialize(task.state).value_or(
                          persistence::PersistenceCodec::Defaults())
                  };
              },
              [this] { restoreDelivery.set(); }) {
        statusOut.send("status", "initializing");
    }

private:
    bool EnsureInitializedState() {
        if (!host.InstanceId().empty()) return true;
        auto defaults = persistence::PersistenceCodec::Defaults();
        if (host.Restore(
            std::move(defaults.eq),
            defaults.processor,
            0,
            defaults.instanceId
        )) {
            return true;
        }
        debugOut.send("error", "persistence_defaults_failed", host.LastRestoreError());
        return false;
    }

    void FinalizeInitialization() {
        persistenceReadyRequested = false;
        if (!EnsureInitializedState()) return;
        PublishReadyState();
        PublishAllSnapshots();
        PublishHostInitializedEvent();
        PublishPersistence();
    }

    void DeliverRestore() {
        const auto completion = restoreExecutor.TakeCompletion();
        if (!completion || completion->error || !completion->result) {
            restorePending = false;
            debugOut.send("error", "invalid_restore");
            if (persistenceReadyRequested) FinalizeInitialization();
            return;
        }

        restorePending = false;
        auto state = std::move(completion->result->state);
        if (!host.Restore(std::move(state.eq), state.processor, 0, state.instanceId)) {
            if (!EnsureInitializedState()) {
                debugOut.send("error", "persistence_defaults_failed", host.LastRestoreError());
            }
            if (persistenceReadyRequested) FinalizeInitialization();
            return;
        }
        if (ready) PublishAllSnapshots();
        if (persistenceReadyRequested) FinalizeInitialization();
    }

    void ApplyCommand(const std::optional<messaging::AtomList>& atoms) {
        if (!atoms) {
            debugOut.send("error", "invalid_atom");
            return;
        }
        const auto decoded = messaging::CommandCodec::Decode(*atoms);
        if (!decoded.Succeeded()) {
            debugOut.send("error", decoded.error.code);
            return;
        }
        if (!ready) {
            debugOut.send("error", "host_not_ready");
            return;
        }
        try {
            const auto revisionBefore = host.Revision();
            const auto isHistoryRestore =
                std::holds_alternative<domain::UndoHistoryCommand>(decoded.command) ||
                std::holds_alternative<domain::RedoHistoryCommand>(decoded.command) ||
                std::holds_alternative<domain::RestoreHistoryOperationCommand>(decoded.command);
            suppressContinuousStatePublication = IsContinuousParameterCommand(decoded.command);
            suppressDspStatePublication = IsDspIndependentCommand(decoded.command);
            host.Handle(decoded.command);
            suppressContinuousStatePublication = false;
            suppressDspStatePublication = false;
            if (isHistoryRestore && host.Revision() != revisionBefore) {
                PublishAllSnapshots();
            }
            if (host.Revision() != revisionBefore &&
                IsContinuousParameterCommand(decoded.command)) {
                PublishParameterUpdate(decoded.command, host.Revision());
            }
            if (host.Revision() != revisionBefore) SyncCoordinator();
        }
        catch (const std::exception&) {
            suppressContinuousStatePublication = false;
            suppressDspStatePublication = false;
            debugOut.send("error", "host_command_failed");
        }
    }

    void ApplyLinkedCommand(const domain::Command& command) {
        if (!ready) return;
        const auto revisionBefore = host.Revision();
        suppressContinuousStatePublication = true;
        host.Handle(command);
        suppressContinuousStatePublication = false;
        if (host.Revision() == revisionBefore) return;
        PublishParameterUpdate(command, host.Revision());
        SyncCoordinator();
    }

    void ApplyLinkedFilterGesture(const host::LinkedFilterGesture& gesture) {
        if (runtimeId.empty() || gesture.sourceRuntimeId == runtimeId) return;
        const auto bank = std::find_if(host.Eq().State().banks.begin(), host.Eq().State().banks.end(),
            [&gesture](const auto& candidate) { return candidate.linkId == gesture.linkId; });
        const auto definition = domain::FilterDefinitions().find(gesture.filterId);
        if (bank == host.Eq().State().banks.end() || definition == domain::FilterDefinitions().end()) return;
        const auto* filter = bank->FindFilter(gesture.filterId);
        if (!filter || gesture.parameterIndex >= filter->values.size() ||
            gesture.parameterIndex >= definition->second.parameters.size()) return;
        const auto& range = definition->second.parameters[gesture.parameterIndex].range;
        const auto value = range.Denormalize(range.Normalize(filter->values[gesture.parameterIndex]) +
            gesture.targetNormalized - gesture.sourceNormalized);
        ApplyLinkedCommand(domain::SetEqParameterIndexCommand{
            {}, { bank->bankId }, { gesture.filterId }, gesture.parameterIndex, value
        });
        eventOut.send("eq_preview", bank->bankId, gesture.filterId,
            static_cast<long>(gesture.parameterIndex), value);
    }

    void ApplyLinkedProcessorGesture(const host::LinkedProcessorGesture& gesture) {
        if (runtimeId.empty() || gesture.sourceRuntimeId == runtimeId) return;
        const auto bank = std::find_if(host.Eq().State().banks.begin(), host.Eq().State().banks.end(),
            [&gesture](const auto& candidate) { return candidate.linkId == gesture.linkId; });
        const auto definition = models::ParameterDefinitions().find(gesture.device + "." + gesture.parameter);
        const auto current = ReadProcessorValue(gesture.device, gesture.parameter);
        if (bank == host.Eq().State().banks.end() || definition == models::ParameterDefinitions().end() || !current) return;
        const auto value = definition->second.Denormalize(definition->second.Normalize(*current) +
            gesture.targetNormalized - gesture.sourceNormalized);
        if (gesture.device == "input_gain" || gesture.device == "output_gain") {
            ApplyLinkedCommand(domain::SetGainParameterCommand{ {},
                gesture.device == "input_gain" ? domain::GainStage::Input : domain::GainStage::Output, value });
        } else if (gesture.device == "compressor") {
            long filterId = 0;
            std::string parameter;
            if (ParseDetectorParameter(gesture.parameter, filterId, parameter)) {
                ApplyLinkedCommand(domain::SetCompressorDetectorParameterCommand{ {}, filterId, parameter, value });
            } else ApplyLinkedCommand(domain::SetCompressorParameterCommand{ {}, gesture.parameter, value });
        } else if (gesture.device == "saturator") {
            long filterId = 0;
            std::string parameter;
            if (ParseDetectorParameter(gesture.parameter, filterId, parameter)) {
                ApplyLinkedCommand(domain::SetSaturatorDetectorParameterCommand{ {}, filterId, parameter, value });
            } else ApplyLinkedCommand(domain::SetSaturatorParameterCommand{ {}, gesture.parameter, value });
        }
        eventOut.send("coordinator_processor_preview", gesture.device, gesture.parameter, value);
    }

    std::optional<double> ReadProcessorValue(
        const std::string& device,
        const std::string& parameter
    ) const {
        return ReadProcessorValue({ host.InputGain().State(), host.Compressor().State(),
            host.Saturator().State(), host.OutputGain().State() }, device, parameter);
    }

    static std::optional<double> ReadProcessorValue(
        const models::ProcessorState& processor,
        const std::string& device,
        const std::string& parameter
    ) {
        if (device == "input_gain" && parameter == "gain") return processor.inputGain.gainDb;
        if (device == "output_gain" && parameter == "gain") return processor.outputGain.gainDb;
        if (device == "compressor") {
            const auto& state = processor.compressor;
            if (parameter == "attack") return state.attackMs;
            if (parameter == "release") return state.releaseMs;
            if (parameter == "threshold") return state.thresholdDb;
            if (parameter == "output") return state.outputDb;
            if (parameter == "mix") return state.mix;
            long detectorFilterId = 0;
            std::string detectorParameter;
            if (ParseDetectorParameter(parameter, detectorFilterId, detectorParameter)) {
                const auto& filter = state.detectorFilters[static_cast<std::size_t>(detectorFilterId - 1)];
                if (detectorParameter == "gain") return filter.gainDb;
                if (detectorParameter == "frequency") return filter.frequencyHz;
                if (detectorParameter == "q") return filter.q;
            }
        }
        if (device == "saturator") {
            const auto& state = processor.saturator;
            if (parameter == "saturation") return state.saturation;
            if (parameter == "output") return state.outputDb;
            long detectorFilterId = 0;
            std::string detectorParameter;
            if (ParseDetectorParameter(parameter, detectorFilterId, detectorParameter)) {
                const auto& filter = state.detectorFilters[static_cast<std::size_t>(detectorFilterId - 1)];
                if (detectorParameter == "gain") return filter.gainDb;
                if (detectorParameter == "frequency") return filter.frequencyHz;
                if (detectorParameter == "q") return filter.q;
            }
        }
        return std::nullopt;
    }

    static bool ParseDetectorParameter(
        const std::string& parameter,
        long& filterId,
        std::string& parameterName
    ) {
        constexpr std::string_view prefix = "detector.";
        if (!parameter.starts_with(prefix) || parameter.size() < prefix.size() + 3) return false;
        const auto separator = parameter.find('.', prefix.size());
        if (separator == std::string::npos) return false;
        const auto id = std::strtol(parameter.c_str() + prefix.size(), nullptr, 10);
        const auto name = parameter.substr(separator + 1);
        if ((name != "gain" && name != "frequency" && name != "q") || id < 1 || id > 2) return false;
        filterId = id;
        parameterName = name;
        return true;
    }

    static bool IsContinuousParameterCommand(const domain::Command& command) {
        return std::holds_alternative<domain::SetEqParameterCommand>(command) ||
            std::holds_alternative<domain::SetEqParameterIndexCommand>(command) ||
            std::holds_alternative<domain::SetGainParameterCommand>(command) ||
            std::holds_alternative<domain::SetCompressorParameterCommand>(command) ||
            std::holds_alternative<domain::SetSaturatorParameterCommand>(command);
    }

    bool IsDspIndependentCommand(const domain::Command& command) const {
        if (std::holds_alternative<domain::SetEqBankLinkCommand>(command)) return true;
        return std::holds_alternative<domain::SelectEqBankCommand>(command) &&
            !host.Eq().State().solo;
    }

    void PublishParameterUpdate(
        const domain::Command& command,
        domain::StoreRevision revision
    ) {
        const auto send = [this, revision](domain::ParameterUpdatedEvent update) {
            update.revision = revision;
            messaging::EventCodec::Send(
                domain::Event{ std::move(update) }, { nextEventId++ },
                [this](const messaging::AtomList& atoms) {
                    eventOut.send(maxadapter::AtomAdapter::Write(atoms));
                });
        };
        std::visit([&send](const auto& value) {
            using Command = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<Command, domain::SetEqParameterCommand>) {
                send({ 0, "eq", static_cast<long>(value.bankId.value),
                    static_cast<long>(value.filterId.value), value.parameter, value.value });
            }
            else if constexpr (std::is_same_v<Command, domain::SetEqParameterIndexCommand>) {
                const auto definition = domain::FilterDefinitions().find(
                    static_cast<long>(value.filterId.value));
                if (definition == domain::FilterDefinitions().end() ||
                    value.parameterIndex >= definition->second.parameters.size()) return;
                send({ 0, "eq", static_cast<long>(value.bankId.value),
                    static_cast<long>(value.filterId.value),
                    definition->second.parameters[value.parameterIndex].name, value.value });
            }
            else if constexpr (std::is_same_v<Command, domain::SetGainParameterCommand>) {
                send({ 0, value.stage == domain::GainStage::Input ? "input_gain" : "output_gain",
                    0, 0, "gain", value.gainDb });
            }
            else if constexpr (std::is_same_v<Command, domain::SetCompressorParameterCommand>) {
                send({ 0, "compressor", 0, 0, value.parameter, value.value });
            }
            else if constexpr (std::is_same_v<Command, domain::SetSaturatorParameterCommand>) {
                send({ 0, "saturator", 0, 0, value.parameter, value.value });
            }
        }, command);
    }

    void PublishReadyState() {
        if (!ready) {
            ready = true;
            statusOut.send("status", "ready");
        }
    }

    void PublishHostInitializedEvent() {
        if (hostInitializedPublished) return;
        hostInitializedPublished = true;
        messaging::EventCodec::Send(
            domain::HostInitializedEvent{ host.Revision() },
            { nextEventId++ },
            [this](const messaging::AtomList& atoms) {
                eventOut.send(maxadapter::AtomAdapter::Write(atoms));
            });
    }

    void SyncCoordinator() {
        if (runtimeId.empty() || !ready) return;
        host::LinkCoordinator::Instance().Upsert({
            runtimeId,
            trackName,
            trackOrder,
            host.Revision(),
            host.Eq().State(),
            {
                host.InputGain().State(), host.Compressor().State(),
                host.Saturator().State(), host.OutputGain().State()
            }
        });
    }

    void PublishCoordinatorDirectory() {
        const auto entries = host::LinkCoordinator::Instance().Entries();
        atoms values;
        values.emplace_back("coordinator_directory");
        values.emplace_back(static_cast<long>(entries.size()));
        for (const auto& entry : entries) {
            values.emplace_back(entry.runtimeId);
            values.emplace_back(entry.trackName);
            values.emplace_back(entry.trackOrder);
            values.emplace_back(entry.eq.selectedBankId);
            for (long bankId = models::EqSnapshot::SystemBankId;
                 bankId <= models::EqSnapshot::LastUserBankId;
                 ++bankId) {
                const auto* bank = entry.eq.FindBank(bankId);
                values.emplace_back(bank ? bank->linkId : "-");
                const auto occupied = bank && std::any_of(
                    bank->filters.begin(), bank->filters.end(),
                    [](const auto& filter) { return !filter.bypass; });
                values.emplace_back(occupied ? 1 : 0);
            }
        }
        eventOut.send(values);
    }

    void PublishCoordinatorLimits() {
        const auto* bank = host.Eq().State().SelectedBank();
        if (!bank) return;
        const auto entries = host::LinkCoordinator::Instance().Entries();
        const auto linked = !bank->linkId.empty() &&
            host::LinkCoordinator::Instance().Members(bank->linkId).size() >= 2;
        const auto effectiveRange = [linked](const models::ParameterRange& range,
            double sourceValue, const std::vector<double>& memberValues) {
            if (!linked) return std::pair{ range.minimum, range.maximum };
            const auto source = range.Normalize(sourceValue);
            double minimumDelta = -std::numeric_limits<double>::infinity();
            double maximumDelta = std::numeric_limits<double>::infinity();
            for (const auto value : memberValues) {
                minimumDelta = std::max(minimumDelta, -range.Normalize(value));
                maximumDelta = std::min(maximumDelta, 1.0 - range.Normalize(value));
            }
            return std::pair{
                range.Denormalize(std::max(0.0, source + minimumDelta)),
                range.Denormalize(std::min(1.0, source + maximumDelta))
            };
        };

        for (const auto& [key, range] : models::ParameterDefinitions()) {
            const auto separator = key.find('.');
            if (separator == std::string::npos) continue;
            const auto device = key.substr(0, separator);
            const auto parameter = key.substr(separator + 1);
            const auto source = ReadProcessorValue(device, parameter);
            if (!source) continue;
            std::vector<double> memberValues;
            for (const auto& entry : entries) {
                const auto memberBank = std::find_if(entry.eq.banks.begin(), entry.eq.banks.end(),
                    [&bank](const auto& candidate) { return candidate.linkId == bank->linkId; });
                if (memberBank == entry.eq.banks.end() || entry.runtimeId == runtimeId) continue;
                const auto value = ReadProcessorValue(entry.processor, device, parameter);
                if (value) memberValues.push_back(*value);
            }
            const auto [minimum, maximum] = effectiveRange(range, *source, memberValues);
            atoms values;
            values.emplace_back("coordinator_processor_limits");
            values.emplace_back(device);
            values.emplace_back(parameter);
            values.emplace_back(minimum);
            values.emplace_back(maximum);
            eventOut.send(values);
        }

        for (const auto& [filterId, definition] : domain::FilterDefinitions()) {
            for (std::size_t parameterIndex = 0; parameterIndex < definition.parameters.size(); ++parameterIndex) {
                const auto& range = definition.parameters[parameterIndex].range;
                const auto* sourceFilter = bank->FindFilter(filterId);
                if (!sourceFilter || parameterIndex >= sourceFilter->values.size()) continue;
                std::vector<double> memberValues;
                for (const auto& entry : entries) {
                    for (const auto& candidate : entry.eq.banks) {
                        if (entry.runtimeId == runtimeId || candidate.linkId != bank->linkId) continue;
                        const auto* filter = candidate.FindFilter(filterId);
                        if (filter && parameterIndex < filter->values.size()) memberValues.push_back(filter->values[parameterIndex]);
                    }
                }
                const auto [minimum, maximum] = effectiveRange(range, sourceFilter->values[parameterIndex], memberValues);
                atoms values;
                values.emplace_back("coordinator_filter_limits");
                values.emplace_back(bank->bankId);
                values.emplace_back(filterId);
                values.emplace_back(static_cast<long>(parameterIndex));
                values.emplace_back(minimum);
                values.emplace_back(maximum);
                eventOut.send(values);
            }
        }
    }

    void PublishAllSnapshots() {
        eventOut.send(maxadapter::AtomAdapter::Write(
            messaging::SnapshotCodec::EncodeDevice(host.InstanceId())));
        PublishEqSnapshot();
        PublishDspSnapshot();
        PublishProcessorSnapshot();
        eqSnapshotDirty = false;
        dspSnapshotDirty = false;
        processorSnapshotDirty = false;
        SyncCoordinator();
        PublishCoordinatorDirectory();
    }

    void PublishEqSnapshot() {
        eventOut.send(maxadapter::AtomAdapter::Write(
            messaging::SnapshotCodec::EncodeEq(host.Eq().State(), host.Revision())));
    }

    void PublishDspSnapshot() {
        eventOut.send(maxadapter::AtomAdapter::Write(
            messaging::SnapshotCodec::EncodeDsp({
                host.Revision(), host.Eq().State(), {
                    host.InputGain().State(), host.Compressor().State(),
                    host.Saturator().State(), host.OutputGain().State()
                }
            })));
    }

    void PublishProcessorSnapshot() {
        eventOut.send(maxadapter::AtomAdapter::Write(
            messaging::SnapshotCodec::EncodeProcessor({
                host.InputGain().State(), host.Compressor().State(),
                host.Saturator().State(), host.OutputGain().State()
            }, host.Revision())));
    }

    void PublishPersistence() {
        persistence::PersistedDeviceState persisted{
            persistence::PersistenceCodec::SchemaVersion,
            host.InstanceId(),
            host.Eq().State(),
            { host.InputGain().State(), host.Compressor().State(),
                host.Saturator().State(), host.OutputGain().State() }
        };
        maxadapter::MaxDictionarySerializer::Serialize(
            persistence::PersistenceCodec::Serialize(persisted),
            [this](const c74::min::atom& dictionary) {
                persistenceOut.send("dictionary", dictionary);
            });
    }

    void PublishContinuousConfirmations() {
        if (continuousEqConfirmationDirty) PublishEqSnapshot();
        if (continuousProcessorConfirmationDirty) PublishProcessorSnapshot();
        continuousEqConfirmationDirty = false;
        continuousProcessorConfirmationDirty = false;
    }

    void ScheduleStatePublication(const std::string& storeName) {
        if (!ready) return;

        if (!suppressContinuousStatePublication) {
            if (storeName == "eq") eqSnapshotDirty = true;
            else processorSnapshotDirty = true;
            if (!suppressDspStatePublication) dspSnapshotDirty = true;
            if (!stateDeliveryScheduled) {
                stateDeliveryScheduled = true;
                stateDelivery.set();
            }
        }
        else if (storeName == "eq") continuousEqConfirmationDirty = true;
        else continuousProcessorConfirmationDirty = true;

        persistenceDirty = true;
        persistenceDelivery.delay(PersistenceDebounceMilliseconds);
    }

    consolidator::host::DeviceHost host;
    workflows::LatestWorkflowExecutor<RestoreTask, RestoreResult> restoreExecutor;
    static constexpr double PersistenceDebounceMilliseconds = 100.0;
    long nextEventId = 1;
    bool ready = false;
    bool hostInitializedPublished = false;
    bool restorePending = false;
    bool persistenceReadyRequested = false;
    bool stateDeliveryScheduled = false;
    bool eqSnapshotDirty = false;
    bool dspSnapshotDirty = false;
    bool processorSnapshotDirty = false;
    bool persistenceDirty = false;
    bool suppressContinuousStatePublication = false;
    bool suppressDspStatePublication = false;
    bool continuousEqConfirmationDirty = false;
    bool continuousProcessorConfirmationDirty = false;
    std::string runtimeId;
    std::string trackName;
    long trackOrder = 0;
};

MIN_EXTERNAL_CUSTOM(ConsolidatorDeviceHost, consolidator.devicehost);
