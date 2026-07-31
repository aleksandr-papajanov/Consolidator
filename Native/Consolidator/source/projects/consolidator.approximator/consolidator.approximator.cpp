#include "c74_min.h"

#include "ApproximatorOutputs.h"
#include "AtomAdapter.h"
#include "AtomMessage.h"
#include "EqMatchWorkflow.h"
#include "EventCodec.h"
#include "Settings/AnalysisOptions.h"
#include "Settings/FilterOptions.h"
#include "SnapshotCodec.h"
#include "Workflows/LatestWorkflowExecutor.h"

#include <algorithm>
#include <exception>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>

using namespace c74::min;
using namespace consolidator;

class ConsolidatorApproximator : public object<ConsolidatorApproximator> {
private:
    struct WorkerResult final {
        domain::DspSnapshot snapshot;
        double loss = 0.0;
        long sessionId = 0;
        long bankId = 0;
        std::string error;
        bool completed = false;
        bool cancelled = false;
    };

    struct FitTask final {
        dsp::Curve target;
        domain::DspSnapshot snapshot;
        std::map<long, models::FilterDefinition> definitions;
        domain::FitTargetKind targetKind = domain::FitTargetKind::Residual;
        long sessionId = 0;
        long bankId = 0;
    };

public:
    MIN_DESCRIPTION{ "Consolidator EQ curve matcher." };
    MIN_TAGS{ "eq, optimizer" };
    MIN_AUTHOR{ "Oleksandr Papaianov" };

    inlet<> commands{
        this,
        "(message) snapshot 1 host eq|processor <revision> <state>; event 1 host <eventId> fit.requested <sessionId> <bankId> <residual|absolute> <pointCount> <curve...>"
    };
    outlet<> commandsOut{
        this,
        "(anything) command 1 approximator <requestId> fit.complete <sessionId> <bankId> <loss> <filterCount> <filters...> <processor fields>; command 1 approximator <requestId> fit.fail <sessionId> <error>"
    };
    outlet<> statusOut{ this, "(anything) status initialized|idle|ready|processing eq|error <code>" };
    outlet<> debugOut{ this, "(anything) diagnostics: fit_started <sessionId> <bankId>; loss <value>; error <code>" };

    queue<> fitCompleted{
        this,
        MIN_FUNCTION {
            DeliverResult();
            return {};
        }
    };

    ConsolidatorApproximator()
        : fitExecutor(
            [this](const FitTask& task, const workflows::WorkflowCancellation& cancellation) {
                return RunFit(task, cancellation);
            },
            [this] { fitCompleted.set(); }) {}

    message<> list{
        this,
        "list",
        "Receive Host snapshots and events",
        MIN_FUNCTION {
            if (inlet != 0) return {};
            const auto atoms = maxadapter::AtomAdapter::Read(args);
            if (messaging::AtomMessage::HasSnapshotStore(atoms, "eq") ||
                messaging::AtomMessage::HasSnapshotStore(atoms, "processor")) {
                ApplySnapshot(atoms);
            }
            else if (messaging::AtomMessage::HasCategory(atoms, "event")) ApplyEvent(atoms);
            return {};
        }
    };

    message<> snapshotMessage{
        this,
        "snapshot",
        "Apply an EQ or processor state snapshot",
        MIN_FUNCTION {
            if (inlet != 0) return {};
            auto atoms = maxadapter::AtomAdapter::Read(args);
            if (atoms) atoms->insert(atoms->begin(), "snapshot");
            if (messaging::AtomMessage::HasSnapshotStore(atoms, "eq") ||
                messaging::AtomMessage::HasSnapshotStore(atoms, "processor")) {
                ApplySnapshot(atoms);
            }
            return {};
        }
    };

    message<> eventMessage{
        this,
        "event",
        "Receive fit request and completion events from DeviceHost",
        MIN_FUNCTION {
            if (inlet != 0) return {};
            auto atoms = maxadapter::AtomAdapter::Read(args);
            if (atoms) atoms->insert(atoms->begin(), "event");
            ApplyEvent(atoms);
            return {};
        }
    };

    message<> loadbang{
        this,
        "loadbang",
        MIN_FUNCTION {
            SetReady(false, true);
            return {};
        }
    };

private:
    void ApplySnapshot(const std::optional<messaging::AtomList>& atoms) {
        if (!atoms || atoms->size() < 5 ||
            !std::holds_alternative<std::string>((*atoms)[3]) ||
            !std::holds_alternative<std::int64_t>((*atoms)[4])) {
            debugOut.send("error", "invalid_fit_state_snapshot");
            return;
        }

        const auto store = std::get<std::string>((*atoms)[3]);
        const auto revision = static_cast<domain::StoreRevision>(
            std::get<std::int64_t>((*atoms)[4]));
        if (store == "eq") {
            auto eq = messaging::SnapshotCodec::DecodeEq(*atoms);
            if (!eq || !eq->SelectedBank()) {
                debugOut.send("error", "invalid_eq_snapshot");
                return;
            }
            latestEq = std::move(*eq);
        } else if (store == "processor") {
            auto processor = messaging::SnapshotCodec::DecodeProcessor(*atoms);
            if (!processor) {
                debugOut.send("error", "invalid_processor_snapshot");
                return;
            }
            latestProcessor = std::move(*processor);
        } else {
            debugOut.send("error", "unsupported_fit_state_snapshot");
            return;
        }
        latestRevision = std::max(latestRevision, revision);
        if (latestEq && latestProcessor) {
            latestSnapshot = domain::DspSnapshot{
                latestRevision, *latestEq, *latestProcessor
            };
        }
        if (!initialized && latestSnapshot) {
            initialized = true;
            statusOut.send("status", "initialized");
        }
        UpdateReady();
    }

    void ApplyEvent(const std::optional<messaging::AtomList>& atoms) {
        if (!atoms) return;
        const auto decoded = messaging::EventCodec::Decode(*atoms);
        if (!decoded.Succeeded()) {
            debugOut.send("error", decoded.error.code);
            return;
        }

        if (const auto* request = std::get_if<domain::FitRequestedEvent>(&decoded.event)) {
            StartFit(*request);
            return;
        }

        const auto* operation = std::get_if<domain::OperationChangedEvent>(&decoded.event);
        if (!operation || operation->operation != "fit.eq") return;
        if (operation->status == domain::OperationStatus::Completed ||
            operation->status == domain::OperationStatus::Failed ||
            operation->status == domain::OperationStatus::Cancelled ||
            operation->status == domain::OperationStatus::Idle) {
            fitExecutor.Cancel();
            running = false;
            activeSessionId = 0;
            UpdateReady();
        }
    }

    void StartFit(const domain::FitRequestedEvent& request) {
        if (running) {
            SendFailure(static_cast<long>(request.sessionId.value), "fit_worker_busy");
            return;
        }
        if (!latestSnapshot || !latestSnapshot->eq.FindBank(static_cast<long>(request.bankId.value))) {
            SendFailure(static_cast<long>(request.sessionId.value), "fit_snapshot_unavailable");
            return;
        }
        if (request.curveDb.size() != settings::AnalysisOptions::DefaultCurvePointCount) {
            SendFailure(static_cast<long>(request.sessionId.value), "invalid_fit_curve");
            return;
        }

        const auto target = dsp::Curve::FromValues(request.curveDb);
        auto snapshot = *latestSnapshot;
        snapshot.eq.selectedBankId = static_cast<long>(request.bankId.value);
        const auto definitions = settings::FilterOptions::EqDefinitions();
        const auto sessionId = static_cast<long>(request.sessionId.value);
        const auto bankId = static_cast<long>(request.bankId.value);

        activeSessionId = sessionId;
        running = true;
        SetReady(false);
        statusOut.send("status", "processing", "eq");
        debugOut.send("fit_started", sessionId, bankId);

        fitExecutor.Submit(latestRevision, {
            std::move(target), std::move(snapshot), definitions, request.targetKind, sessionId, bankId
        });
    }

    void DeliverResult() {
        const auto completion = fitExecutor.TakeCompletion();
        if (!completion) return;
        running = false;
        if (completion->error || !completion->result) {
            SendFailure(activeSessionId, "curve_fit_failed");
            return;
        }
        auto result = std::move(*completion->result);

        if (result.cancelled) {
            UpdateReady();
            return;
        }
        if (!result.completed || !result.error.empty()) {
            SendFailure(result.sessionId, result.error.empty() ? "curve_fit_failed" : result.error.c_str());
            return;
        }

        auto* bank = result.snapshot.eq.SelectedBank();
        if (!bank || bank->bankId != result.bankId) {
            SendFailure(result.sessionId, "invalid_fit_result");
            return;
        }

        debugOut.send("loss", result.loss);
        ApproximatorOutputs{ commandsOut, statusOut, debugOut }.SendFitResult(
            bank->filters, result.snapshot.processor, result.sessionId,
            bank->bankId,
            result.loss);
    }

    WorkerResult RunFit(
        const FitTask& task,
        const workflows::WorkflowCancellation& cancellation
    ) {
        WorkerResult result;
        result.sessionId = task.sessionId;
        result.bankId = task.bankId;
        result.snapshot = task.snapshot;
        if (cancellation.IsRequested()) {
            result.cancelled = true;
            return result;
        }
        try {
            const auto fit = eqWorkflow.Run(task.target, task.snapshot, task.definitions, task.targetKind);
            result.cancelled = cancellation.IsRequested();
            if (!result.cancelled) {
                result.snapshot = fit.snapshot;
                result.loss = fit.loss;
                result.completed = true;
            }
        }
        catch (const std::exception& exception) {
            result.cancelled = cancellation.IsRequested();
            if (!result.cancelled) result.error = exception.what();
        }
        catch (...) {
            result.cancelled = cancellation.IsRequested();
            if (!result.cancelled) result.error = "curve_fit_failed";
        }
        return result;
    }

    void SendFailure(long sessionId, const char* error) {
        ApproximatorOutputs{ commandsOut, statusOut, debugOut }.SendFitFailure(sessionId, error);
    }

    void UpdateReady() {
        SetReady(initialized && latestSnapshot && !running);
    }

    void SetReady(bool value, bool force = false) {
        if (!force && ready == value) return;
        ready = value;
        statusOut.send("status", value ? "ready" : "idle", value ? 1L : 0L);
    }

    EqMatchWorkflow eqWorkflow;
    std::optional<domain::DspSnapshot> latestSnapshot;
    std::optional<domain::EqState> latestEq;
    std::optional<domain::ProcessorState> latestProcessor;
    domain::StoreRevision latestRevision = 0;
    workflows::LatestWorkflowExecutor<FitTask, WorkerResult> fitExecutor;
    long activeSessionId = 0;
    bool initialized = false;
    bool running = false;
    bool ready = false;
};

MIN_EXTERNAL_CUSTOM(ConsolidatorApproximator, consolidator.approximator);
