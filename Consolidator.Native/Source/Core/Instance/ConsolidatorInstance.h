#pragma once

#include <atomic>
#include <array>
#include <cstddef>
#include <memory>
#include "Core/Commands/Commands.h"
#include "Core/Commands/SpscCommandQueue.h"
#include "Core/State/InstanceState.h"
#include "Core/Notifications/Notifications.h"

namespace consolidator::dsp
{
class DspChain;
}

namespace consolidator::core
{

class ConsolidatorInstance;

void HandleStateCommand(
    ConsolidatorInstance& instance,
    const StateCommand& command);

class ConsolidatorInstance
{
public:
    ConsolidatorInstance();
    ~ConsolidatorInstance();

    ConsolidatorInstance(const ConsolidatorInstance&) = delete;
    ConsolidatorInstance& operator=(const ConsolidatorInstance&) = delete;
    ConsolidatorInstance(ConsolidatorInstance&&) = delete;
    ConsolidatorInstance& operator=(ConsolidatorInstance&&) = delete;

    void Process(const double* mainInput,
                 const double* referenceInput,
                 double* mainOutput,
                 double* referenceOutput,
                 std::size_t frameCount);

    void EnqueueCommand(Command command);

    [[nodiscard]] InstanceId GetInstanceId() const noexcept;
    [[nodiscard]] InstanceState& GetState() noexcept { return state_; }
    [[nodiscard]] const InstanceState& GetState() const noexcept { return state_; }
    [[nodiscard]] dsp::DspChain& GetDspChain() noexcept;
    void QueueStateResponse(StateResponse response) noexcept;

private:
    friend class InstanceCoordinator;
    friend void HandleStateCommand(
        ConsolidatorInstance& instance,
        const StateCommand& command);

    [[nodiscard]] bool EnqueueLocalCommand(Command command);
    [[nodiscard]] std::optional<StateResponse> TryDequeueResponse();
    void RecordLocalQueueOverflow() noexcept;
    void RecordResponseQueueOverflow() noexcept;
    void StorePendingResponse(StateResponse response) noexcept;
    void ProcessCommandQueue();
    void HandleCommand(const Command& command);

    static constexpr std::size_t kChannelCount = 2;

    std::unique_ptr<dsp::DspChain> dspChain_;
    InstanceState state_;
    SpscCommandQueue<Command, 128> commandQueue_;
    std::atomic<std::size_t> localQueueOverflowCount_{0};
    std::atomic<std::size_t> responseQueueOverflowCount_{0};
    SpscCommandQueue<StateResponse, 8> responseQueue_;
    std::array<StateResponse, 8> pendingResponses_{};
    std::size_t pendingResponseCount_{0};
};

} // namespace consolidator::core
