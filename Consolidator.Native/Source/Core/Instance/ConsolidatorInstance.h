#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include "Core/Commands/Commands.h"
#include "Core/Commands/SpscCommandQueue.h"
#include "Core/State/InstanceState.h"
#include "Core/Notifications/Notifications.h"
#include "Core/Parameters/RoutedParameterChange.h"

namespace consolidator::dsp
{
class DspChain;
}

namespace consolidator::core
{

class ConsolidatorInstance;

void HandleReadStateCommand(
    ConsolidatorInstance& instance,
    const ReadStateCommand& command);

void HandleChangeDspParameterCommand(
    ConsolidatorInstance& instance,
    const ChangeDspParameterCommand& command);

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
    [[nodiscard]] std::optional<StateResponse> TryDequeueResponse();
    void RecordResponseQueueOverflow() noexcept;

private:
    friend class InstanceCoordinator;
    friend void HandleReadStateCommand(
        ConsolidatorInstance& instance,
        const ReadStateCommand& command);
    friend void HandleChangeDspParameterCommand(
        ConsolidatorInstance& instance,
        const ChangeDspParameterCommand& command);

    [[nodiscard]] bool EnqueueLocalCommand(Command command);
    void RecordLocalQueueOverflow() noexcept;
    void ProcessCommandQueue();
    void HandleCommand(const Command& command);

    static constexpr std::size_t kChannelCount = 2;

    std::unique_ptr<dsp::DspChain> dspChain_;
    InstanceState state_;
    SpscCommandQueue<Command, 128> commandQueue_;
    std::atomic<std::size_t> localQueueOverflowCount_{0};
    std::atomic<std::size_t> responseQueueOverflowCount_{0};
    SpscCommandQueue<StateResponse, 32> responseQueue_;
    std::optional<StateResponse> pendingResponse_;
};

} // namespace consolidator::core
