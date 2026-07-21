#include "c74_min.h"

#include "ApproximatorOutputs.h"
#include "AtomAdapter.h"
#include "AtomMessage.h"
#include "DeviceOptimizer.h"
#include "EventCodec.h"
#include "FitAudioBuffer.h"
#include "OfflineFitEvaluator.h"
#include "SnapshotCodec.h"

#include <atomic>
#include <exception>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <variant>

using namespace c74::min;
using namespace consolidator;

class ConsolidatorApproximator :
    public object<ConsolidatorApproximator>,
    public sample_operator<4, 0> {
public:
    MIN_DESCRIPTION{ "Consolidator offline full-chain metric approximator." };
    MIN_TAGS{ "audio, optimizer" };
    MIN_AUTHOR{ "Oleksandr Papaianov" };

    inlet<> currentLeft{ this, "(signal) pre-DSP current left", "signal" };
    inlet<> currentRight{ this, "(signal) pre-DSP current right", "signal" };
    inlet<> referenceLeft{ this, "(signal) reference left", "signal" };
    inlet<> referenceRight{ this, "(signal) reference right", "signal" };
    inlet<> commands{
        this,
        "(message) snapshot 1 host dsp <revision> <complete DSP state>; event 1 host <eventId> operation.changed fit <sessionId> <status> <progress> [error]"
    };

    outlet<> commandsOut{
        this,
        "(anything) command 1 approximator <requestId> fit.complete <sessionId> <bankId> <loss> <filterCount> <filters...> <processor fields>; command 1 approximator <requestId> fit.fail <sessionId> <error>"
    };
    outlet<> statusOut{ this, "(anything) status: status initialized|idle|ready|capturing|processing|error" };
    outlet<> debugOut{ this, "(anything) diagnostics: error <code>; loss <value>" };

    queue<> captureCompleted{
        this,
        MIN_FUNCTION {
            StartWorker();
            return {};
        }
    };

    queue<> fitCompleted{
        this,
        MIN_FUNCTION {
            DeliverResult();
            return {};
        }
    };

    message<> list{ this, "list", "Receive a complete Host atom message", MIN_FUNCTION {
        if (inlet != 4) return {};
        const auto atoms = maxadapter::AtomAdapter::Read(args);
        if (messaging::AtomMessage::HasSnapshotStore(atoms, "dsp")) ApplyDspSnapshot(atoms);
        else if (messaging::AtomMessage::HasCategory(atoms, "event")) ApplyEvent(atoms);
        return {};
    }};

    message<> snapshotMessage{ this, "snapshot", "Receive a complete DSP snapshot", MIN_FUNCTION {
        if (inlet != 4) return {};
        auto atoms = maxadapter::AtomAdapter::Read(args);
        if (atoms) atoms->insert(atoms->begin(), "snapshot");
        if (messaging::AtomMessage::HasSnapshotStore(atoms, "dsp")) ApplyDspSnapshot(atoms);
        return {};
    }};

    message<> eventMessage{ this, "event", "Receive a Host operation event", MIN_FUNCTION {
        if (inlet != 4) return {};
        auto atoms = maxadapter::AtomAdapter::Read(args);
        if (atoms) atoms->insert(atoms->begin(), "event");
        ApplyEvent(atoms);
        return {};
    }};

    message<> dspsetup{ this, "dspsetup", MIN_FUNCTION {
        if (!args.empty()) {
            sampleRate = static_cast<double>(args[0]);
            capture.Prepare(sampleRate);
        }
        return {};
    }};

    message<> loadbang{ this, "loadbang", MIN_FUNCTION {
        capture.Prepare(sampleRate);
        SetReady(false, true);
        return {};
    }};

    samples<0> operator()(sample currentL, sample currentR, sample referenceL, sample referenceR) {
        if (!capturing.load(std::memory_order_acquire)) return {};
        const auto index = captureIndex.fetch_add(1, std::memory_order_acq_rel);
        if (index >= capture.Size()) return {};
        capture.Write(index, currentL, currentR, referenceL, referenceR);
        if (index + 1 == capture.Size()) {
            capturing.store(false, std::memory_order_release);
            captureCompleted.set();
        }
        return {};
    }

    ~ConsolidatorApproximator() override {
        cancelRequested.store(true, std::memory_order_release);
        if (worker.joinable()) worker.join();
    }

private:
    struct WorkerResult final {
        domain::DspSnapshot snapshot;
        double loss = 0.0;
        long sessionId = 0;
        std::string error;
        bool cancelled = false;
    };

    void ApplyDspSnapshot(const std::optional<messaging::AtomList>& atoms) {
        const auto snapshot = atoms ? messaging::SnapshotCodec::DecodeDsp(*atoms) : std::nullopt;
        if (!snapshot) {
            debugOut.send("error", "invalid_dsp_snapshot");
            return;
        }
        latestDspSnapshot = *snapshot;
        if (!hostInitialized) {
            hostInitialized = true;
            statusOut.send("status", "initialized");
        }
        UpdateReady();
    }

    void ApplyEvent(const std::optional<messaging::AtomList>& atoms) {
        if (!atoms) return;
        const auto decoded = messaging::EventCodec::Decode(*atoms);
        if (!decoded.Succeeded()) return;
        if (std::holds_alternative<domain::HostInitializedEvent>(decoded.event)) {
            hostInitialized = true;
            UpdateReady();
            return;
        }
        const auto* operation = std::get_if<domain::OperationChangedEvent>(&decoded.event);
        if (!operation || operation->operation != "fit") return;
        if (operation->status == domain::OperationStatus::Starting) {
            activeSessionId = static_cast<long>(operation->sessionId.value);
            StartCapture();
            return;
        }
        if (operation->status == domain::OperationStatus::Cancelled ||
            operation->status == domain::OperationStatus::Failed ||
            operation->status == domain::OperationStatus::Idle) {
            CancelFit();
            return;
        }
        if (operation->status == domain::OperationStatus::Completed) {
            activeSessionId = 0;
            workerRunning.store(false, std::memory_order_release);
            UpdateReady();
        }
    }

    void StartCapture() {
        if (!latestDspSnapshot || capture.Size() == 0 || workerRunning.load()) {
            ApproximatorOutputs{ commandsOut, statusOut, debugOut }
                .SendFitFailure(activeSessionId, "fit_not_ready");
            return;
        }
        fitSnapshot = *latestDspSnapshot;
        captureIndex.store(0, std::memory_order_release);
        cancelRequested.store(false, std::memory_order_release);
        workerRunning.store(true, std::memory_order_release);
        capturing.store(true, std::memory_order_release);
        SetReady(false);
        statusOut.send("status", "capturing");
    }

    void StartWorker() {
        if (worker.joinable()) worker.join();
        if (cancelRequested.load() || !fitSnapshot || activeSessionId < 1) {
            workerRunning.store(false, std::memory_order_release);
            UpdateReady();
            return;
        }
        statusOut.send("status", "processing");
        const auto snapshot = *fitSnapshot;
        const auto sessionId = activeSessionId;
        try {
            worker = std::thread([this, snapshot, sessionId]() {
                WorkerResult result;
                result.sessionId = sessionId;
                try {
                    DeviceOptimizer optimizer;
                    auto optimized = optimizer.Optimize(snapshot, capture, cancelRequested);
                    result.cancelled = cancelRequested.load(std::memory_order_acquire);
                    if (!result.cancelled) {
                        result.snapshot = std::move(optimized.snapshot);
                        result.loss = optimized.loss;
                    }
                }
                catch (const std::exception& exception) {
                    result.cancelled = cancelRequested.load(std::memory_order_acquire);
                    if (!result.cancelled) {
                        result.error = std::string{ "offline_fit_exception:" } + exception.what();
                    }
                }
                catch (...) {
                    result.cancelled = cancelRequested.load(std::memory_order_acquire);
                    if (!result.cancelled) result.error = "offline_fit_exception:unknown";
                }
                {
                    std::lock_guard<std::mutex> lock(resultMutex);
                    pendingResult = std::move(result);
                }
                fitCompleted.set();
            });
        }
        catch (const std::exception& exception) {
            workerRunning.store(false, std::memory_order_release);
            const auto error = std::string{ "fit_worker_start_failed:" } + exception.what();
            ApproximatorOutputs{ commandsOut, statusOut, debugOut }.SendFitFailure(
                sessionId,
                error.c_str());
        }
    }

    void DeliverResult() {
        if (worker.joinable()) worker.join();
        std::optional<WorkerResult> result;
        {
            std::lock_guard<std::mutex> lock(resultMutex);
            result = std::move(pendingResult);
            pendingResult.reset();
        }
        workerRunning.store(false, std::memory_order_release);
        if (!result || result->cancelled) {
            UpdateReady();
            return;
        }
        if (!result->error.empty()) {
            ApproximatorOutputs{ commandsOut, statusOut, debugOut }
                .SendFitFailure(result->sessionId, result->error.c_str());
            return;
        }
        const auto* bank = result->snapshot.eq.SelectedBank();
        if (!bank) {
            ApproximatorOutputs{ commandsOut, statusOut, debugOut }
                .SendFitFailure(result->sessionId, "invalid_fit_result");
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

    void CancelFit() {
        capturing.store(false, std::memory_order_release);
        cancelRequested.store(true, std::memory_order_release);
        activeSessionId = 0;
        if (!workerRunning.load(std::memory_order_acquire)) UpdateReady();
    }

    void UpdateReady() {
        SetReady(hostInitialized && latestDspSnapshot && capture.Size() > 0 &&
            !capturing.load(std::memory_order_acquire) &&
            !workerRunning.load(std::memory_order_acquire));
    }

    void SetReady(bool available, bool force = false) {
        if (!force && readyAvailable == available) return;
        readyAvailable = available;
        ApproximatorOutputs{ commandsOut, statusOut, debugOut }.Ready(available);
    }

    double sampleRate = 48000.0;
    FitAudioBuffer capture;
    std::optional<domain::DspSnapshot> latestDspSnapshot;
    std::optional<domain::DspSnapshot> fitSnapshot;
    std::atomic<std::size_t> captureIndex{ 0 };
    std::atomic<bool> capturing{ false };
    std::atomic<bool> workerRunning{ false };
    std::atomic<bool> cancelRequested{ false };
    std::thread worker;
    std::mutex resultMutex;
    std::optional<WorkerResult> pendingResult;
    long activeSessionId = 0;
    bool hostInitialized = false;
    bool readyAvailable = false;
};

MIN_EXTERNAL_CUSTOM(ConsolidatorApproximator, consolidator.approximator);
