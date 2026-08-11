#include "Core/Instance/Queues/RuntimeUpdateMailbox.h"
#include "Core/Queues/ConcurrentQueue.h"
#include "Core/Queues/SpscQueue.h"
#include "Support/StateBuilders.h"
#include "Support/TestFramework.h"

#include <array>
#include <string>

using namespace consolidator;

TEST_CASE("SpscQueue preserves order and reserves one ring slot")
{
    core::SpscQueue<int, 3> queue;
    EXPECT_TRUE(queue.TryEnqueue(10));
    EXPECT_TRUE(queue.TryEnqueue(20));
    EXPECT_FALSE(queue.TryEnqueue(30));
    EXPECT_EQ(queue.TryDequeue(), 10);
    EXPECT_EQ(queue.TryDequeue(), 20);
    EXPECT_FALSE(queue.TryDequeue().has_value());
}

TEST_CASE("ConcurrentQueue exposes FIFO state")
{
    core::ConcurrentQueue<std::string> queue;
    EXPECT_FALSE(queue.HasCommands());
    queue.Enqueue("first");
    queue.Enqueue("second");
    EXPECT_TRUE(queue.HasCommands());
    EXPECT_EQ(queue.TryDequeue(), std::string{"first"});
    EXPECT_EQ(queue.TryDequeue(), std::string{"second"});
    EXPECT_FALSE(queue.HasCommands());
}

TEST_CASE("RuntimeUpdateMailbox coalesces parameter writes by path")
{
    core::RuntimeUpdateMailbox mailbox;
    const auto gain = test::DevicePath(
        core::InstanceId{1}, dsp::DeviceId::MainInputGain, dsp::ParameterId::Gain);
    mailbox.RegisterPath(gain);
    const std::array updates{
        core::ParameterUpdate{gain, dsp::ParameterVariant{3.0f}, 10},
        core::ParameterUpdate{gain, dsp::ParameterVariant{6.0f}, 11}};
    mailbox.EnqueueParameters(updates);

    core::ParameterUpdateBatch batch;
    EXPECT_TRUE(mailbox.ConsumeLatest(batch));
    EXPECT_EQ(batch.count, 1U);
    EXPECT_EQ(std::get<float>(batch.updates[0].value), 6.0f);
    EXPECT_EQ(batch.updates[0].revision, 2U);
    EXPECT_FALSE(mailbox.ConsumeLatest(batch));
}

TEST_CASE("Runtime control mailbox keeps properties in separate slots")
{
    core::RuntimeUpdateMailbox mailbox;
    const auto detector = test::RuntimeTarget(test::DetectorPath(
        core::InstanceId{2}, dsp::DeviceId::Compressor, dsp::ParameterId::Listen));
    mailbox.RegisterControlPath(detector, core::RuntimeProperty::Active);
    mailbox.RegisterControlPath(detector, core::RuntimeProperty::Listen);
    const std::array updates{
        core::RuntimeControlUpdate{detector, core::RuntimeProperty::Active, false, 1},
        core::RuntimeControlUpdate{detector, core::RuntimeProperty::Listen, true, 2}};
    mailbox.EnqueueRuntimeControls(updates);

    core::RuntimeControlBatch batch;
    EXPECT_TRUE(mailbox.ConsumeControlLatest(batch));
    EXPECT_EQ(batch.count, 2U);
    EXPECT_FALSE(batch.updates[0].value);
    EXPECT_TRUE(batch.updates[1].value);
}

TEST_MAIN()
