#pragma once

#include <cstddef>
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
    // Enqueues a non-coalescable real-time reset event.
    void EnqueueCommand(ResetDspCommand command);

    [[nodiscard]] std::optional<CommandResponse> TryDequeueResponse();
    [[nodiscard]] bool HasResponse() const;
    [[nodiscard]] bool SetResponseNotifier(ResponseNotifier notifier);
    void ShutdownResponseNotifier() noexcept;

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
    static void NotifyResponseAvailable(ResponseNotifierHandle notifier);

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

    static constexpr std::size_t kChannelCount = 2;

    std::unique_ptr<dsp::DspChain> dspChain_;
    StateStore stateStore_;
    RuntimeUpdateMailbox runtimeUpdateMailbox_;
    // SpscQueue reserves one ring slot, so 17 storage slots provide the
    // documented capacity of 16 pending realtime commands.
    SpscQueue<RealtimeCommand, 17> realtimeCommandQueue_;
    ConcurrentQueue<CommandResponse> responseQueue_;
    ResponseNotifierHandle responseNotifier_;
    bool outputEnabled_ = true;
    bool initialized_ = false;
};

} // namespace consolidator::core
