#include "c74_min.h"

#include "ApproximatorOutputs.h"
#include "AtomAdapter.h"
#include "AtomMessage.h"
#include "EqMatchWorkflow.h"
#include "EventCodec.h"
#include "Settings/AnalysisOptions.h"
#include "Settings/FilterOptions.h"
#include "SnapshotCodec.h"

#include <atomic>
#include <exception>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <variant>

using namespace c74::min;
using namespace consolidator;

class ConsolidatorApproximator : public object<ConsolidatorApproximator> {
public:
    MIN_DESCRIPTION{ "Consolidator EQ curve matcher." };
    MIN_TAGS{ "eq, optimizer" };
    MIN_AUTHOR{ "Oleksandr Papaianov" };

    inlet<> commands{
        this,
        "(message) snapshot 1 host dsp <revision> <state>; event 1 host <eventId> fit.requested <sessionId> <bankId> <pointCount> <curve...>"
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

    message<> list{
        this,
        "list",
        "Receive Host snapshots and events",
        MIN_FUNCTION {
            if (inlet != 0) return {};
            const auto atoms = maxadapter::AtomAdapter::Read(args);
            if (messaging::AtomMessage::HasSnapshotStore(atoms, "dsp")) ApplySnapshot(atoms);
            else if (messaging::AtomMessage::HasCategory(atoms, "event")) ApplyEvent(atoms);
            return {};
        }
    };

    message<> snapshotMessage{
        this,
        "snapshot",
        "Apply a complete DSP snapshot",
        MIN_FUNCTION {
            if (inlet != 0) return {};
            auto atoms = maxadapter::AtomAdapter::Read(args);
            if (atoms) atoms->insert(atoms->begin(), "snapshot");
            if (messaging::AtomMessage::HasSnapshotStore(atoms, "dsp")) ApplySnapshot(atoms);
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

    ~ConsolidatorApproximator() override {
        cancelRequested.store(true, std::memory_order_release);
        if (worker.joinable()) worker.join();
    }

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

    void ApplySnapshot(const std::optional<messaging::AtomList>& atoms) {
        const auto snapshot = atoms ? messaging::SnapshotCodec::DecodeDsp(*atoms) : std::nullopt;
        if (!snapshot || !snapshot->eq.SelectedBank()) {
            debugOut.send("error", "invalid_dsp_snapshot");
            return;
        }

        latestSnapshot = *snapshot;
        if (!initialized) {
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
            cancelRequested.store(true, std::memory_order_release);
            running.store(false, std::memory_order_release);
            activeSessionId = 0;
            UpdateReady();
        }
    }

    void StartFit(const domain::FitRequestedEvent& request) {
        if (running.load(std::memory_order_acquire)) {
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
        cancelRequested.store(false, std::memory_order_release);
        running.store(true, std::memory_order_release);
        SetReady(false);
        statusOut.send("status", "processing", "eq");
        debugOut.send("fit_started", sessionId, bankId);

        if (worker.joinable()) worker.join();
        worker = std::thread([this, target, snapshot, definitions, sessionId, bankId]() mutable {
            WorkerResult result;
            result.sessionId = sessionId;
            result.bankId = bankId;
            result.snapshot = snapshot;
            try {
                const auto fit = eqWorkflow.Run(target, snapshot, definitions);
                result.snapshot = fit.snapshot;
                result.loss = fit.loss;
                result.completed = true;
            }
            catch (const std::exception& exception) {
                result.cancelled = cancelRequested.load(std::memory_order_acquire);
                if (!result.cancelled) result.error = exception.what();
            }
            catch (...) {
                result.cancelled = cancelRequested.load(std::memory_order_acquire);
                if (!result.cancelled) result.error = "curve_fit_failed";
            }
            {
                std::lock_guard<std::mutex> lock(resultMutex);
                pendingResult = std::move(result);
            }
            fitCompleted.set();
        });
    }

    void DeliverResult() {
        if (worker.joinable()) worker.join();
        std::optional<WorkerResult> result;
        {
            std::lock_guard<std::mutex> lock(resultMutex);
            result = std::move(pendingResult);
            pendingResult.reset();
        }

        if (!result || result->cancelled) {
            running.store(false, std::memory_order_release);
            UpdateReady();
            return;
        }
        if (!result->completed || !result->error.empty()) {
            SendFailure(result->sessionId, result->error.empty() ? "curve_fit_failed" : result->error.c_str());
            return;
        }

        auto* bank = result->snapshot.eq.SelectedBank();
        if (!bank || bank->bankId != result->bankId) {
            SendFailure(result->sessionId, "invalid_fit_result");
            return;
        }

        debugOut.send("loss", result->loss);
        ApproximatorOutputs{ commandsOut, statusOut, debugOut }.SendFitResult(
            bank->filters,
            result->snapshot.processor,
            result->sessionId,
            bank->bankId,
            result->loss);
    }

    void SendFailure(long sessionId, const char* error) {
        ApproximatorOutputs{ commandsOut, statusOut, debugOut }.SendFitFailure(sessionId, error);
    }

    void UpdateReady() {
        SetReady(initialized && latestSnapshot && !running.load(std::memory_order_acquire));
    }

    void SetReady(bool value, bool force = false) {
        if (!force && ready == value) return;
        ready = value;
        statusOut.send("status", value ? "ready" : "idle", value ? 1L : 0L);
    }

    EqMatchWorkflow eqWorkflow;
    std::optional<domain::DspSnapshot> latestSnapshot;
    std::atomic<bool> cancelRequested{ false };
    std::atomic<bool> running{ false };
    std::thread worker;
    std::mutex resultMutex;
    std::optional<WorkerResult> pendingResult;
    long activeSessionId = 0;
    bool initialized = false;
    bool ready = false;
};

MIN_EXTERNAL_CUSTOM(ConsolidatorApproximator, consolidator.approximator);
