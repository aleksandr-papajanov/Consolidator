#pragma once

#include "Core/Coordinator/InstanceCoordinator.h"
#include "Core/Instance/ConsolidatorInstance.h"
#include "Support/StateBuilders.h"
#include "Support/TestFramework.h"

#include <array>
#include <chrono>
#include <cstddef>
#include <memory>
#include <thread>
#include <vector>

namespace consolidator::test
{

class ProtocolDriver
{
public:
    explicit ProtocolDriver(std::size_t instanceCount)
    {
        for (std::size_t index = 0; index < instanceCount; ++index)
        {
            instances_.push_back(std::make_unique<core::ConsolidatorInstance>());
            instances_.back()->Initialize();
        }
    }

    core::ConsolidatorInstance& At(std::size_t index) { return *instances_.at(index); }

    core::StateResponse Write(
        std::size_t sourceIndex,
        core::RequestId requestId,
        core::StateRequestEntries entries)
    {
        At(sourceIndex).EnqueueCommand(core::WriteStateCommand{
            .requestId = requestId,
            .entries = std::move(entries)});
        return Await(requestId);
    }

    core::StateResponse Read(
        std::size_t sourceIndex,
        core::RequestId requestId,
        core::StatePath path)
    {
        At(sourceIndex).EnqueueCommand(core::ReadStateCommand{
            .requestId = requestId,
            .queries = Entries({test::Write(std::move(path), std::monostate{})})});
        return Await(requestId);
    }

    void ProcessAll()
    {
        for (auto& instance : instances_)
        {
            instance->Process(
                mainInput_.data(), referenceInput_.data(),
                mainOutput_.data(), referenceOutput_.data(), kFrameCount);
        }
    }

    std::array<double, 32>& MainInput() noexcept { return mainInput_; }
    std::array<double, 32>& ReferenceInput() noexcept { return referenceInput_; }
    const std::array<double, 32>& MainOutput() const noexcept { return mainOutput_; }
    const std::array<double, 32>& ReferenceOutput() const noexcept { return referenceOutput_; }

private:
    core::StateResponse Await(core::RequestId requestId)
    {
        for (std::size_t attempt = 0; attempt < 500; ++attempt)
        {
            ProcessAll();
            while (auto response = core::InstanceCoordinator::Get().TryDequeueResponse())
            {
                if (response->requestId == requestId)
                {
                    return std::move(*response);
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
        }
        throw std::runtime_error("state protocol response timeout");
    }

    static constexpr std::size_t kFrameCount = 16;
    std::vector<std::unique_ptr<core::ConsolidatorInstance>> instances_;
    std::array<double, 32> mainInput_{};
    std::array<double, 32> referenceInput_{};
    std::array<double, 32> mainOutput_{};
    std::array<double, 32> referenceOutput_{};
};

inline const core::StateEntry& FindEntry(
    const core::StateResponse& response,
    const core::StatePath& path)
{
    for (std::size_t index = 0; index < response.entries.size; ++index)
    {
        if (IsExactPath(response.entries.entries[index].path, path))
        {
            return response.entries.entries[index];
        }
    }
    throw std::runtime_error("expected response entry was not found");
}

} // namespace consolidator::test
