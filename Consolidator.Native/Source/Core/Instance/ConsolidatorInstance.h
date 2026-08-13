#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <utility>

#include "Core/Domain/Commands/StateProtocolCommands.h"
#include "Core/Domain/Commands/RealtimeCommands.h"
#include "Core/Domain/State/StateStore.h"
#include "Core/Instance/Queues/RuntimeUpdateMailbox.h"
#include "Core/Queues/ConcurrentQueue.h"
#include "Core/Queues/SpscQueue.h"
#include "Analysis/AnalysisService.h"
#include "Core/Analysis/AnalysisCurveInputBuilder.h"

namespace consolidator::dsp
{
class DspChain;
}

namespace consolidator::core
{

class StateWriter;
// Represents one processor instance and bridges coordinator state with its DSP chain.
class ConsolidatorInstance
{
  public:
    using ResponseNotifier = std::function<void()>;
    struct ResponseNotifierState
    {
        explicit ResponseNotifierState(ResponseNotifier callback)
            : callback(std::move(callback))
        {
        }

        std::mutex mutex;
        ResponseNotifier callback;
        bool active = true;
    };
    using ResponseNotifierHandle = std::shared_ptr<ResponseNotifierState>;
    using RegistryNotifier = std::function<void(std::uint64_t)>;
    struct RegistryNotifierState
    {
        explicit RegistryNotifierState(RegistryNotifier callback)
            : callback(std::move(callback))
        {
        }

        std::mutex mutex;
        RegistryNotifier callback;
        bool active = true;
    };
    using RegistryNotifierHandle = std::shared_ptr<RegistryNotifierState>;

    ConsolidatorInstance();
    ~ConsolidatorInstance();

    // Registers the instance, paths and initial runtime values before audio starts.
    void Initialize();

    // Prepares the DSP chain for the host sample rate before audio starts.
    void Prepare(double sampleRate);

    ConsolidatorInstance(const ConsolidatorInstance&) = delete;
    ConsolidatorInstance& operator=(const ConsolidatorInstance&) = delete;
    ConsolidatorInstance(ConsolidatorInstance&&) = delete;
    ConsolidatorInstance& operator=(ConsolidatorInstance&&) = delete;

    // Consumes one audio block and applies pending runtime updates before DSP.
    void Process(const double* mainInputLeft,
                 const double* mainInputRight,
                 const double* referenceInputLeft,
                 const double* referenceInputRight,
                 double* mainOutputLeft,
                 double* mainOutputRight,
                 double* referenceOutputLeft,
                 double* referenceOutputRight,
                 std::size_t frameCount);

    void EnqueueCommand(ReadStateCommand command);
    void EnqueueCommand(WriteStateCommand command);
    void EnqueueCommand(ReadRegistryCommand command);
    // Enqueues a non-coalescable real-time reset event.
    void EnqueueCommand(ResetDspCommand command);

    [[nodiscard]] std::optional<CommandResponse> TryDequeueResponse();
    [[nodiscard]] bool HasResponse() const;
    [[nodiscard]] bool SetResponseNotifier(ResponseNotifier notifier);
    [[nodiscard]] bool SetRegistryNotifier(RegistryNotifier notifier);
    void ShutdownNotifiers() noexcept;

    [[nodiscard]] InstanceId GetInstanceId() const noexcept;
    [[nodiscard]] bool IsOutputEnabled() const noexcept
    {
        return outputEnabled_;
    }
    [[nodiscard]] StateStore& GetStateStore() noexcept { return stateStore_; }
    [[nodiscard]] const StateStore& GetStateStore() const noexcept { return stateStore_; }
    [[nodiscard]] dsp::DspChain& GetDspChain() noexcept;

  private:
    friend class InstanceCoordinator;
    friend class CommandRouter;
    friend class StateWriter;

    void EnqueueResponse(CommandResponse response);
    [[nodiscard]] ResponseNotifierHandle GetResponseNotifierHandle() const noexcept;
    [[nodiscard]] RegistryNotifierHandle GetRegistryNotifierHandle() const noexcept;
    static void NotifyResponseAvailable(ResponseNotifierHandle notifier);
    static void NotifyRegistryChanged(
        RegistryNotifierHandle notifier,
        std::uint64_t revision);

    // Enqueues coordinator-owned latest-value updates for the audio thread.
    void EnqueueParameterUpdates(std::span<const ParameterUpdate> updates);
    void EnqueueRuntimeUpdates(
        std::span<const RuntimeControlUpdate> updates);

    // Enqueues a reset route without coalescing it.
    [[nodiscard]] bool EnqueueRealtimeCommand(const StatePath& target);
    void ConsumeParameterUpdates();
    void ConsumeRuntimeUpdates();
    void ProcessRealtimeCommands();
    void ApplyOutputGate(
        double* mainOutput,
        std::size_t sampleCount) const;

    // Registers all paths and sends the complete initial DSP runtime snapshot.
    void PublishInitialRuntimeState();
    // Called by the coordinator after a committed analysis-relevant change.
    void PublishAnalysisState();
    // Publishes a new curve input after Prepare changes the sample rate.
    void RefreshAnalysisState();

    static constexpr std::size_t kChannelCount = 2;

    std::unique_ptr<dsp::DspChain> dspChain_;
    StateStore stateStore_;
    analysis::AnalysisHandle analysisHandle_;
    AnalysisCurveInputBuilder analysisCurveInputBuilder_;
    std::atomic<double> sampleRate_{0.0};
    std::atomic<std::uint64_t> sampleRateRevision_{0};
    std::uint64_t nextAnalysisRevision_ = 1;
    std::uint64_t publishedSampleRateRevision_ = 0;
    // The instance is the single Max-facing consumer of each spectrum stream.
    RuntimeUpdateMailbox runtimeUpdateMailbox_;
    // SpscQueue reserves one ring slot, so 17 storage slots provide the
    // documented capacity of 16 pending realtime commands.
    SpscQueue<RealtimeCommand, 17> realtimeCommandQueue_;
    ConcurrentQueue<CommandResponse> responseQueue_;
    ResponseNotifierHandle responseNotifier_;
    RegistryNotifierHandle registryNotifier_;
    bool outputEnabled_ = true;
    bool initialized_ = false;
};

} // namespace consolidator::core
