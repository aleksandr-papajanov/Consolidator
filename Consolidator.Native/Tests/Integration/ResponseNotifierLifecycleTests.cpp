#include "Core/Instance/ConsolidatorInstance.h"

#include <chrono>
#include <condition_variable>
#include <future>
#include <mutex>
#include <utility>

#include "Support/TestFramework.h"

using namespace consolidator;

TEST_CASE("Response notifier shutdown waits for an active callback")
{
    core::ConsolidatorInstance instance;
    std::mutex mutex;
    std::condition_variable condition;
    bool callbackEntered = false;
    bool releaseCallback = false;
    bool releaseQueueSet = false;
    bool passedAcceptanceCheck = false;
    bool queueSet = false;

    EXPECT_TRUE(instance.SetResponseNotifier([&]
    {
        std::unique_lock lock{mutex};
        callbackEntered = true;
        condition.notify_one();
        condition.wait(lock, [&] { return releaseCallback; });
        passedAcceptanceCheck = true;
        condition.notify_one();
        condition.wait(lock, [&] { return releaseQueueSet; });
        queueSet = true;
    }));
    instance.Initialize();

    core::ReadStateCommand command;
    command.requestId = 1;
    command.instanceId = instance.GetInstanceId();
    (void)command.queries.TryAppend(core::StateEntry{
        core::StatePath::Instance(instance.GetInstanceId()), {}});
    (void)instance.EnqueueCommand(std::move(command));

    {
        std::unique_lock lock{mutex};
        EXPECT_TRUE(condition.wait_for(
            lock,
            std::chrono::seconds{2},
            [&] { return callbackEntered; }));
    }

    auto shutdown = std::async(
        std::launch::async,
        [&] { instance.ShutdownNotifiers(); });

    EXPECT_EQ(
        shutdown.wait_for(std::chrono::milliseconds{20}),
        std::future_status::timeout);

    {
        std::lock_guard lock{mutex};
        releaseCallback = true;
    }
    condition.notify_one();
    {
        std::unique_lock lock{mutex};
        EXPECT_TRUE(condition.wait_for(
            lock,
            std::chrono::seconds{2},
            [&] { return passedAcceptanceCheck; }));
    }
    EXPECT_EQ(
        shutdown.wait_for(std::chrono::milliseconds{20}),
        std::future_status::timeout);
    {
        std::lock_guard lock{mutex};
        releaseQueueSet = true;
    }
    condition.notify_one();
    EXPECT_EQ(shutdown.wait_for(std::chrono::seconds{2}),
              std::future_status::ready);
    EXPECT_TRUE(passedAcceptanceCheck);
    EXPECT_TRUE(queueSet);
}

TEST_MAIN()
