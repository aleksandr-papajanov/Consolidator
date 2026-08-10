#pragma once

#include <cstddef>
#include <memory>
#include <span>

#include "Core/Domain/Commands/StateProtocolCommands.h"
#include "Core/Domain/State/StateStore.h"
#include "Core/Instance/Queues/DspUpdateMailbox.h"

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

    [[nodiscard]] InstanceId GetInstanceId() const noexcept;
    [[nodiscard]] StateStore& GetStateStore() noexcept { return stateStore_; }
    [[nodiscard]] const StateStore& GetStateStore() const noexcept { return stateStore_; }
    [[nodiscard]] dsp::DspChain& GetDspChain() noexcept;

private:
    friend class InstanceCoordinator;
    friend class CommandRouter;
    friend class StateWriter;

    // Publishes coordinator-owned updates into the audio-thread mailbox.
    void PublishDspUpdates(std::span<const DspUpdate> updates);
    // Registers all paths and sends the complete initial DSP runtime snapshot.
    void PublishInitialRuntimeState();
    static constexpr std::size_t kChannelCount = 2;

    std::unique_ptr<dsp::DspChain> dspChain_;
    StateStore stateStore_;
    DspUpdateMailbox dspUpdateMailbox_;
    std::uint64_t nextDspRevision_ = 0;
    bool initialized_ = false;
};

} // namespace consolidator::core
