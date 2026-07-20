#include "c74_min.h"

#include "ApproximatorCurveStore.h"
#include "ApproximatorOutputs.h"
#include "AtomAdapter.h"
#include "AtomMessage.h"
#include "DSP/Eq/EqRuntime.h"
#include "EqOptimizer.h"
#include "EventCodec.h"
#include "SnapshotCodec.h"
#include "Settings/AudioOptions.h"

#include <atomic>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <variant>

using namespace c74::min;
using namespace consolidator;

class ConsolidatorApproximator : public object<ConsolidatorApproximator> {
public:
    MIN_DESCRIPTION{ "Consolidator EQ curve approximator." };
    MIN_TAGS{ "audio, eq, optimizer" };
    MIN_AUTHOR{ "Oleksandr Papaianov" };

    inlet<> commands{
        this,
        "(message) commands: snapshot 1 host eq <revision> <selectedBank> <bankCount> <banks...>; event 1 host <eventId> operation.changed fit ..."
    };
    inlet<> inputCurve{ this, "(message) target difference curve: list <dB...>; clear_difference" };

    outlet<> commandsOut{
        this,
        "(anything) commands: command 1 approximator <requestId> fit.complete <sessionId> <bankId> <loss> <filters...>; command 1 approximator <requestId> fit.fail <sessionId> <error>"
    };
    outlet<> statusOut{ this, "(anything) status: status initialized|idle|ready|processing|error <code>" };
    outlet<> debugOut{ this, "(anything) diagnostics: error <code>, loss <value>" };

    queue<> fitDelivery{
        this,
        MIN_FUNCTION {
            DeliverFitResult();
            return {};
        }
    };

    message<> list{
        this,
        "list",
        "Receive analysis curves",
        MIN_FUNCTION {
            if (inlet == 0) {
                const auto atoms = maxadapter::AtomAdapter::Read(args);
                if (messaging::AtomMessage::HasSnapshotStore(atoms, "eq")) ApplySnapshot(atoms);
                else if (messaging::AtomMessage::HasCategory(atoms, "event")) ApplyEvent(atoms);
                return {};
            }
            if (inlet == 1) curveStore.SetTarget(args);
            UpdateReady();
            return {};
        }
    };

    message<> eventMessage{
        this,
        "event",
        "Apply a Host operation event",
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

    message<> clearDifference{
        this,
        "clear_difference",
        "Clear the live difference curve",
        MIN_FUNCTION {
            if (inlet != 1) return {};
            curveStore.ClearTarget();
            UpdateReady();
            return {};
        }
    };

    message<> snapshotMessage{
        this,
        "snapshot",
        "Apply a complete EQ snapshot",
        MIN_FUNCTION {
            if (inlet != 0) {
                debugOut.send("error", "invalid_snapshot_inlet");
                return {};
            }
            auto atoms = maxadapter::AtomAdapter::Read(args);
            if (atoms) atoms->insert(atoms->begin(), "snapshot");
            if (messaging::AtomMessage::HasSnapshotStore(atoms, "eq")) ApplySnapshot(atoms);
            return {};
        }
    };

    ~ConsolidatorApproximator() override {
        if (fitWorker.joinable()) fitWorker.join();
    }

private:
    using Definitions = EqOptimizer::Definitions;

    void ApplyEvent(const std::optional<messaging::AtomList>& atoms) {
        if (!atoms) return;
        const auto decoded = messaging::EventCodec::Decode(*atoms);
        if (!decoded.Succeeded()) return;
        if (std::holds_alternative<domain::HostInitializedEvent>(decoded.event)) {
            MarkHostInitialized();
            return;
        }
        const auto* operation = std::get_if<domain::OperationChangedEvent>(&decoded.event);
        if (!operation || operation->operation != "fit") return;
        if (operation->status == domain::OperationStatus::Starting) {
            activeFitSessionId = static_cast<long>(operation->sessionId.value);
            StartFit();
        }
        else if (operation->status == domain::OperationStatus::Completed) {
            fitRunning = false;
            activeFitSessionId = 0;
            SetReady(false, true);
        }
        else if (operation->status == domain::OperationStatus::Failed) {
            fitRunning = false;
            activeFitSessionId = 0;
            if (!operation->error.empty()) {
                ApproximatorOutputs{ commandsOut, statusOut, debugOut }.Error(operation->error.c_str());
            }
        }
        else if (operation->status == domain::OperationStatus::Idle ||
                 operation->status == domain::OperationStatus::Cancelled) {
            activeFitSessionId = 0;
            curveStore.ClearTarget();
            SetReady(false, true);
        }
    }

    void ApplySnapshot(const std::optional<messaging::AtomList>& atoms) {
        const auto snapshot = atoms ? messaging::SnapshotCodec::DecodeEq(*atoms) : std::nullopt;
        if (!snapshot || !snapshot->FindBank(snapshot->selectedBankId)) {
            debugOut.send("error", "invalid_eq_snapshot");
            return;
        }
        MarkHostInitialized();
        definitions = eqRuntime.Definitions();
        fitBankIndex = snapshot->selectedBankId;
        eqRuntime.SetSnapshot(*snapshot);
        curveStore.SetCurrentEq(eqRuntime.BuildBankCurve(fitBankIndex, sampleRate));
        curveStore.ClearTarget();
        UpdateReady();
    }

    void StartFit() {
        ApproximatorOutputs outputs{ commandsOut, statusOut, debugOut };
        if (fitRunning.load()) {
            RejectFit("fit_worker_busy");
            return;
        }
        if (fitBankIndex < 1) {
            RejectFit("no_selected_bank");
            return;
        }
        if (definitions.empty()) {
            RejectFit("no_defined_filters");
            return;
        }
        if (!curveStore.HasTarget()) {
            RejectFit("no_difference_curve");
            return;
        }
        if (!curveStore.HasCurrentEq()) {
            RejectFit("no_current_eq_curve");
            return;
        }
        if (!curveStore.HasCompatibleCurves()) {
            RejectFit("curve_size_mismatch");
            return;
        }

        const auto curve = curveStore.CombinedCurve();
        const auto fitDefinitions = definitions;
        {
            std::lock_guard<std::mutex> lock(fitMutex);
            fitRunning = true;
            activeFitBankIndex = fitBankIndex;
            pendingError.clear();
            pendingResult.reset();
        }
        if (fitWorker.joinable()) fitWorker.join();
        SetReady(false);
        outputs.FitStarted();
        fitWorker = std::thread([this, curve, fitDefinitions]() {
            try {
                const auto result = optimizer.Fit(curve, fitDefinitions);
                std::lock_guard<std::mutex> lock(fitMutex);
                pendingResult = result;
            }
            catch (const std::exception& error) {
                std::lock_guard<std::mutex> lock(fitMutex);
                pendingError = error.what();
            }
            fitDelivery.set();
        });
    }

    void DeliverFitResult() {
        std::optional<EqOptimizer::FitResult> result;
        std::string error;
        {
            std::lock_guard<std::mutex> lock(fitMutex);
            result = pendingResult;
            error = pendingError;
            pendingResult.reset();
            pendingError.clear();
        }
        if (fitWorker.joinable()) fitWorker.join();
        ApproximatorOutputs outputs{ commandsOut, statusOut, debugOut };
        if (!error.empty()) {
            const auto sessionId = activeFitSessionId.load();
            if (sessionId > 0) outputs.SendFitFailure(sessionId, error.c_str());
        }
        else if (result) {
            outputs.Loss(result->loss);
            const auto sessionId = activeFitSessionId.load();
            if (sessionId > 0) {
                outputs.SendFitResult(
                    definitions, result->solverValues, sessionId, activeFitBankIndex, result->loss);
            }
        }
        if (activeFitSessionId.load() <= 0) {
            fitRunning = false;
            UpdateReady();
        }
    }

    void RejectFit(const char* error) {
        ApproximatorOutputs outputs{ commandsOut, statusOut, debugOut };
        const auto sessionId = activeFitSessionId.load();
        if (sessionId > 0) outputs.SendFitFailure(sessionId, error);
        else outputs.Error(error);
        readyAvailable = false;
    }

    void UpdateReady() {
        SetReady(
            fitBankIndex >= 1 && curveStore.HasCompatibleCurves() && !definitions.empty() && !fitRunning.load()
        );
    }

    void SetReady(bool available, bool force = false) {
        if (!force && fitRunning) available = false;
        if (!force && readyAvailable == available) return;
        readyAvailable = available;
        ApproximatorOutputs{ commandsOut, statusOut, debugOut }.Ready(available);
    }

    void MarkHostInitialized() {
        if (hostInitialized) return;
        hostInitialized = true;
        statusOut.send("status", "initialized");
    }

    ApproximatorCurveStore curveStore;
    Definitions definitions;
    EqOptimizer optimizer;
    consolidator::dsp::EqRuntime eqRuntime;
    double sampleRate = consolidator::settings::AudioOptions::DefaultSampleRateHz;
    std::mutex fitMutex;
    std::thread fitWorker;
    std::atomic<bool> fitRunning = false;
    std::atomic<long> activeFitSessionId = 0;
    std::optional<EqOptimizer::FitResult> pendingResult;
    std::string pendingError;
    bool readyAvailable = false;
    bool hostInitialized = false;
    long fitBankIndex = 0;
    long activeFitBankIndex = 0;
};

MIN_EXTERNAL_CUSTOM(ConsolidatorApproximator, consolidator.approximator);
