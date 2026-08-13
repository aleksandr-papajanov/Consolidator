#pragma once

#include "Core/Coordinator/InstanceCoordinator.h"
#include "Core/Instance/ConsolidatorInstance.h"
#include "Support/StateBuilders.h"
#include "Support/TestFramework.h"

#include <array>
#include <chrono>
#include <cstddef>
#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <thread>
#include <vector>

namespace consolidator::test
{

class ProtocolDriver
{
public:
    explicit ProtocolDriver(
        std::size_t instanceCount,
        core::ConsolidatorInstance::ResponseNotifier notifier = {})
    {
        for (std::size_t index = 0; index < instanceCount; ++index)
        {
            instances_.push_back(std::make_unique<core::ConsolidatorInstance>());
            if (notifier)
            {
                (void)instances_.back()->SetResponseNotifier(notifier);
            }
            instances_.back()->Initialize();
        }
        pendingResponses_.resize(instanceCount);
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
        return Await(sourceIndex, requestId);
    }

    core::StateResponse Read(
        std::size_t sourceIndex,
        core::RequestId requestId,
        core::StatePath path)
    {
        EnqueueRead(sourceIndex, requestId, std::move(path));
        return AwaitRead(sourceIndex, requestId);
    }

    void EnqueueRead(
        std::size_t sourceIndex,
        core::RequestId requestId,
        core::StatePath path)
    {
        At(sourceIndex).EnqueueCommand(core::ReadStateCommand{
            .requestId = requestId,
            .queries = Entries({test::Write(std::move(path), std::monostate{})})});
    }

    core::StateResponse AwaitRead(
        std::size_t sourceIndex,
        core::RequestId requestId)
    {
        return Await(sourceIndex, requestId);
    }

    core::ActionResponse Reset(
        std::size_t sourceIndex,
        core::RequestId requestId,
        core::StatePath target)
    {
        At(sourceIndex).EnqueueCommand(core::ResetDspCommand{
            .requestId = requestId,
            .target = std::move(target)});
        return AwaitAction(sourceIndex, requestId);
    }

    void EnqueueRegistryRead(
        std::size_t sourceIndex,
        core::RequestId requestId)
    {
        At(sourceIndex).EnqueueCommand(core::ReadRegistryCommand{
            .requestId = requestId});
    }

    core::RegistryResponse AwaitRegistry(
        std::size_t sourceIndex,
        core::RequestId requestId)
    {
        return AwaitResponse<core::RegistryResponse>(
            sourceIndex,
            requestId,
            "registry response timeout");
    }

    void ProcessAll()
    {
        for (auto& instance : instances_)
        {
            instance->Process(
                mainInput_.data(), mainInput_.data() + kFrameCount,
                referenceInput_.data(), referenceInput_.data() + kFrameCount,
                mainOutput_.data(), mainOutput_.data() + kFrameCount,
                referenceOutput_.data(), referenceOutput_.data() + kFrameCount,
                kFrameCount);
        }
    }

    std::array<double, 32>& MainInput() noexcept { return mainInput_; }
    std::array<double, 32>& ReferenceInput() noexcept { return referenceInput_; }
    const std::array<double, 32>& MainOutput() const noexcept { return mainOutput_; }
    const std::array<double, 32>& ReferenceOutput() const noexcept { return referenceOutput_; }

private:
    template <typename ResponseType>
    ResponseType AwaitResponse(
        std::size_t sourceIndex,
        core::RequestId requestId,
        const char* timeoutMessage)
    {
        if (auto response = TakePending<ResponseType>(sourceIndex, requestId))
        {
            return std::move(*response);
        }
        for (std::size_t attempt = 0; attempt < 500; ++attempt)
        {
            ProcessAll();
            while (auto response = At(sourceIndex).TryDequeueResponse())
            {
                if (const auto* typedResponse =
                        std::get_if<ResponseType>(&*response);
                    typedResponse != nullptr &&
                    typedResponse->requestId == requestId)
                {
                    return std::move(*typedResponse);
                }
                pendingResponses_.at(sourceIndex).push_back(
                    std::move(*response));
            }
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
        }
        throw std::runtime_error(timeoutMessage);
    }

    core::StateResponse Await(std::size_t sourceIndex, core::RequestId requestId)
    {
        return AwaitResponse<core::StateResponse>(
            sourceIndex,
            requestId,
            "state protocol response timeout");
    }

    core::ActionResponse AwaitAction(
        std::size_t sourceIndex,
        core::RequestId requestId)
    {
        return AwaitResponse<core::ActionResponse>(
            sourceIndex,
            requestId,
            "action response timeout");
    }

    template <typename ResponseType>
    std::optional<ResponseType> TakePending(
        std::size_t sourceIndex,
        core::RequestId requestId)
    {
        auto& pending = pendingResponses_.at(sourceIndex);
        for (auto iterator = pending.begin(); iterator != pending.end(); ++iterator)
        {
            if (const auto* response = std::get_if<ResponseType>(&*iterator);
                response != nullptr && response->requestId == requestId)
            {
                auto result = std::optional<ResponseType>{std::move(*response)};
                pending.erase(iterator);
                return result;
            }
        }
        return std::nullopt;
    }

    static constexpr std::size_t kFrameCount = 16;
    std::vector<std::unique_ptr<core::ConsolidatorInstance>> instances_;
    std::vector<std::deque<core::CommandResponse>> pendingResponses_;
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
