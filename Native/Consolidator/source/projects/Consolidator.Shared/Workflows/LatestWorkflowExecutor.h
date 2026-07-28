#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <exception>
#include <functional>
#include <mutex>
#include <optional>
#include <thread>
#include <utility>

namespace consolidator::workflows {

class WorkflowCancellation final {
public:
    bool IsRequested() const noexcept {
        return stopping->load(std::memory_order_acquire) ||
            latestTaskId->load(std::memory_order_acquire) != taskId;
    }

private:
    template <typename Input, typename Result>
    friend class LatestWorkflowExecutor;

    WorkflowCancellation(
        const std::atomic<std::uint64_t>& latestTaskId,
        const std::atomic<bool>& stopping,
        std::uint64_t taskId
    ) : latestTaskId(&latestTaskId), stopping(&stopping), taskId(taskId) {}

    const std::atomic<std::uint64_t>* latestTaskId = nullptr;
    const std::atomic<bool>* stopping = nullptr;
    std::uint64_t taskId = 0;
};

template <typename Input, typename Result>
class LatestWorkflowExecutor final {
public:
    using Revision = std::uint64_t;
    using Work = std::function<Result(const Input&, const WorkflowCancellation&)>;
    using CompletionNotifier = std::function<void()>;

    struct Completion final {
        std::uint64_t taskId = 0;
        Revision revision = 0;
        std::optional<Result> result;
        std::exception_ptr error;
    };

    explicit LatestWorkflowExecutor(Work work, CompletionNotifier completionNotifier = {})
        : work(std::move(work)), completionNotifier(std::move(completionNotifier)),
          worker([this] { Run(); }) {}

    ~LatestWorkflowExecutor() {
        {
            std::lock_guard<std::mutex> lock(mutex);
            stopping.store(true, std::memory_order_release);
            pending.reset();
        }
        wakeup.notify_one();
        if (worker.joinable()) worker.join();
    }

    LatestWorkflowExecutor(const LatestWorkflowExecutor&) = delete;
    LatestWorkflowExecutor& operator=(const LatestWorkflowExecutor&) = delete;

    std::uint64_t Submit(Revision revision, Input input) {
        const auto taskId = latestTaskId.fetch_add(1, std::memory_order_acq_rel) + 1;
        {
            std::lock_guard<std::mutex> lock(mutex);
            pending = Request{ taskId, revision, std::move(input) };
        }
        wakeup.notify_one();
        return taskId;
    }

    void Cancel() {
        latestTaskId.fetch_add(1, std::memory_order_acq_rel);
        std::lock_guard<std::mutex> lock(mutex);
        pending.reset();
        completion.reset();
    }

    bool IsRunning() const noexcept {
        return running.load(std::memory_order_acquire);
    }

    std::optional<Completion> TakeCompletion() {
        std::lock_guard<std::mutex> lock(mutex);
        auto value = std::move(completion);
        completion.reset();
        return value;
    }

private:
    struct Request final {
        std::uint64_t taskId = 0;
        Revision revision = 0;
        Input input;
    };

    void Run() {
        while (true) {
            std::optional<Request> request;
            {
                std::unique_lock<std::mutex> lock(mutex);
                wakeup.wait(lock, [this] {
                    return stopping.load(std::memory_order_acquire) || pending.has_value();
                });
                if (stopping.load(std::memory_order_acquire)) return;
                request = std::move(pending);
                pending.reset();
                running.store(true, std::memory_order_release);
            }

            Completion next;
            next.taskId = request->taskId;
            next.revision = request->revision;
            const WorkflowCancellation cancellation{ latestTaskId, stopping, request->taskId };
            try {
                next.result.emplace(work(request->input, cancellation));
            }
            catch (...) {
                next.error = std::current_exception();
            }

            auto notify = false;
            {
                std::lock_guard<std::mutex> lock(mutex);
                running.store(false, std::memory_order_release);
                if (!cancellation.IsRequested()) {
                    completion = std::move(next);
                    notify = true;
                }
            }
            if (notify && completionNotifier) completionNotifier();
        }
    }

    Work work;
    CompletionNotifier completionNotifier;
    std::atomic<std::uint64_t> latestTaskId{ 0 };
    std::atomic<bool> stopping{ false };
    std::atomic<bool> running{ false };
    std::mutex mutex;
    std::condition_variable wakeup;
    std::optional<Request> pending;
    std::optional<Completion> completion;
    std::thread worker;
};

} // namespace consolidator::workflows
