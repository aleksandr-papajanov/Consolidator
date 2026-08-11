#pragma once

#include <cstddef>
#include <memory>
#include <span>

#include "Core/Domain/Commands/StateProtocolCommands.h"
#include "Core/Domain/Commands/RealtimeCommands.h"
#include "Core/Domain/State/StateStore.h"
#include "Core/Instance/Queues/RuntimeUpdateMailbox.h"
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
    ConsolidatorInstance();
    ~ConsolidatorInstance();

    // Registers the instance, paths and initial runtime values before audio starts.
    void Initialize();

    ConsolidatorInstance(const ConsolidatorInstance&) = delete;
    ConsolidatorInstance& operator=(const ConsolidatorInstance&) = delete;
    ConsolidatorInstance(ConsolidatorInstance&&) = delete;
    ConsolidatorInstance& operator=(ConsolidatorInstance&&) = delete;

    // Consumes one audio block and applies pending runtime updates before DSP.
    void Process(const double* mainInput,
                 const double* referenceInput,
                 double* mainOutput,
                 double* referenceOutput,
                 std::size_t frameCount);

    void EnqueueCommand(ReadStateCommand command);
    void EnqueueCommand(WriteStateCommand command);
    // Enqueues a non-coalescable real-time reset event.
    void EnqueueCommand(ResetDspCommand command);

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

    // Enqueues coordinator-owned latest-value updates for the audio thread.
    void EnqueueParameterUpdates(std::span<const ParameterUpdate> updates);
    void EnqueueRuntimeUpdates(
        std::span<const RuntimeControlUpdate> updates);

    // Enqueues a reset route without coalescing it.
    void EnqueueRealtimeCommand(const StatePath& target);
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
    SpscQueue<RealtimeCommand, 16> realtimeCommandQueue_;
    bool outputEnabled_ = true;
    bool initialized_ = false;
};

} // namespace consolidator::core
