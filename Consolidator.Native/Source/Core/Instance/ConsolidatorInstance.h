#pragma once

#include <cstddef>
#include <memory>
#include <span>

#include "Core/Domain/Commands/StateProtocolCommands.h"
#include "Core/Domain/State/InstanceState.h"
#include "Core/Domain/State/StateStore.h"
#include "Core/Instance/Queues/DspUpdateMailbox.h"

namespace consolidator::dsp
{
class DspChain;
}

namespace consolidator::core
{

class ConsolidatorInstance;
class ConsolidatorInstance
{
public:
    ConsolidatorInstance();
    ~ConsolidatorInstance();

    void Initialize();

    ConsolidatorInstance(const ConsolidatorInstance&) = delete;
    ConsolidatorInstance& operator=(const ConsolidatorInstance&) = delete;
    ConsolidatorInstance(ConsolidatorInstance&&) = delete;
    ConsolidatorInstance& operator=(ConsolidatorInstance&&) = delete;

    void Process(const double* mainInput,
                 const double* referenceInput,
                 double* mainOutput,
                 double* referenceOutput,
                 std::size_t frameCount);

    void HandleStateCommand(StateCommand command);

    [[nodiscard]] InstanceId GetInstanceId() const noexcept;
    [[nodiscard]] StateStore& GetStateStore() noexcept { return stateStore_; }
    [[nodiscard]] const StateStore& GetStateStore() const noexcept { return stateStore_; }
    [[nodiscard]] dsp::DspChain& GetDspChain() noexcept;

private:
    friend class InstanceCoordinator;
    friend class CommandRouter;

    void PublishDspUpdates(std::span<const DspUpdate> updates);
    void PublishInitialRuntimeState();
    static constexpr std::size_t kChannelCount = 2;

    std::unique_ptr<dsp::DspChain> dspChain_;
    InstanceState state_;
    StateStore stateStore_;
    DspUpdateMailbox dspUpdateMailbox_;
    std::uint64_t nextDspRevision_ = 0;
    bool initialized_ = false;
};

} // namespace consolidator::core
