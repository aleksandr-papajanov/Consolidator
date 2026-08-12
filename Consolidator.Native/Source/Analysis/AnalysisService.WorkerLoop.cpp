#include "Analysis/AnalysisService.h"

#include <chrono>
#include <mutex>
#include <thread>

namespace consolidator::analysis
{

void AnalysisService::WorkerLoop(std::stop_token stopToken)
{
    std::vector<AnalysisHandle> slots;
    while (!stopToken.stop_requested())
    {
        bool didWork = false;
        {
            std::lock_guard lock{slotsMutex_};
            slots = slots_;
        }

        for (const auto& handle : slots)
        {
            didWork = ProcessSlot(*handle) || didWork;
        }

        if (!didWork)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds{2});
        }
        else
        {
            std::this_thread::yield();
        }
    }
}

} // namespace consolidator::analysis
