#pragma once

#include <cstddef>
#include <memory>

#include "Core/Commands/Commands.h"
#include "Core/Instance/Queues/InstanceCommandQueue.h"
#include "Core/Instance/Queues/InstanceResponseQueue.h"
#include "Core/Notifications/Notifications.h"
#include "Core/State/InstanceState.h"

namespace consolidator::dsp
{
class DspChain;
}

namespace consolidator::core
{

class ConsolidatorInstance;
class CommandDeliveryQueue;

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

private:
    friend class InstanceCoordinator;
    friend class CommandDeliveryQueue;

    [[nodiscard]] bool EnqueueLocalCommand(Command command);
    [[nodiscard]] std::optional<StateResponse> TryDequeueResponse();
    void RecordLocalQueueOverflow() noexcept;

    static constexpr std::size_t kChannelCount = 2;

    std::unique_ptr<dsp::DspChain> dspChain_;
    InstanceState state_;
    InstanceCommandQueue commandQueue_;
    InstanceResponseQueue responseQueue_;
};

} // namespace consolidator::core
