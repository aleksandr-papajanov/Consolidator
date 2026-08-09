#include "Core/Commands/Commands.h"
#include "Core/Coordinator/InstanceCoordinator.h"
#include "Core/Instance/ConsolidatorInstance.h"
#include "Core/State/StateProtocol.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <optional>
#include <thread>
#include <utility>
#include <variant>
#include <vector>

namespace
{

using consolidator::core::ConsolidatorInstance;
using consolidator::core::GroupId;
using consolidator::core::InstanceCoordinator;
using consolidator::core::InstanceId;
using consolidator::core::StateCommand;
using consolidator::core::StateEntry;
using consolidator::core::StateOperation;
using consolidator::core::StatePath;
using consolidator::core::StateRequestEntries;
using consolidator::core::StateResponse;
using consolidator::core::StateValue;
using consolidator::dsp::BankId;
using consolidator::dsp::DeviceId;
using consolidator::dsp::ParameterId;
using consolidator::dsp::RouteNodeId;

struct AudioBuffers
{
    static constexpr std::size_t kFrameCount = 16;
    static constexpr std::size_t kChannelCount = 2;
    static constexpr std::size_t kSampleCount = kFrameCount * kChannelCount;

    std::array<double, kSampleCount> mainInput{};
    std::array<double, kSampleCount> referenceInput{};
    std::array<double, kSampleCount> mainOutput{};
    std::array<double, kSampleCount> referenceOutput{};
};

bool IsComplete(const std::vector<StateResponse>& responses)
{
    if (responses.empty())
    {
        return false;
    }

    const auto expectedCount = responses.front().responseCount;
    if (expectedCount == 0 || responses.size() < expectedCount)
    {
        return false;
    }

    std::vector<bool> received(expectedCount, false);
    for (const auto& response : responses)
    {
        assert(response.responseCount == expectedCount);
        assert(response.responseIndex < expectedCount);
        received[response.responseIndex] = true;
    }

    return std::ranges::all_of(received, [](bool value) { return value; });
}

std::vector<StateResponse> Pump(
    const std::vector<ConsolidatorInstance*>& instances,
    std::uint64_t requestId)
{
    std::vector<StateResponse> responses;
    AudioBuffers buffers;

    for (std::size_t attempt = 0; attempt < 500; ++attempt)
    {
        for (auto* instance : instances)
        {
            assert(instance != nullptr);
            instance->Process(
                buffers.mainInput.data(),
                buffers.referenceInput.data(),
                buffers.mainOutput.data(),
                buffers.referenceOutput.data(),
                AudioBuffers::kFrameCount);
        }

        while (auto response = InstanceCoordinator::Get().TryDequeueResponse())
        {
            if (response->requestId == requestId)
            {
                responses.push_back(std::move(*response));
            }
        }

        if (IsComplete(responses))
        {
            std::ranges::sort(
                responses,
                {},
                &StateResponse::responseIndex);

            assert(responses.back().isFinal);
            return responses;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }

    assert(false && "state response timeout");
    return {};
}

std::vector<StateResponse> Write(
    const std::vector<ConsolidatorInstance*>& instances,
    ConsolidatorInstance& source,
    std::uint64_t requestId,
    StateRequestEntries entries)
{
    source.EnqueueCommand(StateCommand{
        StateOperation::Write,
        {requestId, source.GetInstanceId(), std::move(entries)}});

    return Pump(instances, requestId);
}

std::vector<StateResponse> Read(
    const std::vector<ConsolidatorInstance*>& instances,
    ConsolidatorInstance& source,
    std::uint64_t requestId,
    StatePath path)
{
    StateRequestEntries queries;
    assert(queries.TryAppend(StateEntry{
        std::move(path),
        StateValue{std::monostate{}}}));

    source.EnqueueCommand(StateCommand{
        StateOperation::Read,
        {requestId, source.GetInstanceId(), std::move(queries)}});

    return Pump(instances, requestId);
}

StateEntry DspWrite(
    InstanceId instanceId,
    DeviceId device,
    ParameterId parameter,
    float value,
    std::initializer_list<RouteNodeId> nodes = {})
{
    StatePath route{device, parameter};
    for (const auto node : nodes)
    {
        route = route.WithNode(node);
    }

    return {
        StatePath::DspParameter(instanceId, route),
        StateValue{value}};
}

StateEntry GroupWrite(
    InstanceId instanceId,
    BankId bankId,
    std::optional<GroupId> groupId)
{
    return {
        StatePath::BankGroup(instanceId, bankId),
        groupId ? StateValue{*groupId} : StateValue{std::monostate{}}};
}

bool IsExactPath(const StatePath& lhs, const StatePath& rhs)
{
    return lhs.Matches(rhs) && rhs.Matches(lhs);
}

const StateEntry& OnlyEntry(const std::vector<StateResponse>& responses)
{
    assert(responses.size() == 1);
    assert(responses.front().responseCount == 1);
    assert(responses.front().responseIndex == 0);
    assert(responses.front().isFinal);
    assert(!responses.front().truncated);
    assert(responses.front().entries.size == 1);
    return responses.front().entries.entries[0];
}

const StateEntry& OnlyEntry(
    const std::vector<StateResponse>& responses,
    const StatePath& expectedPath)
{
    const auto& entry = OnlyEntry(responses);
    assert(IsExactPath(entry.path, expectedPath));
    return entry;
}

void AssertFloatValue(
    const std::vector<StateResponse>& responses,
    const StatePath& expectedPath,
    float expectedValue)
{
    const auto& entry = OnlyEntry(responses, expectedPath);
    assert(std::holds_alternative<float>(entry.value));
    assert(std::get<float>(entry.value) == expectedValue);
}

void AssertGroupValue(
    const std::vector<StateResponse>& responses,
    const StatePath& expectedPath,
    std::optional<GroupId> expectedValue)
{
    const auto& entry = OnlyEntry(responses, expectedPath);
    if (expectedValue)
    {
        assert(std::holds_alternative<GroupId>(entry.value));
        assert(std::get<GroupId>(entry.value) == *expectedValue);
    }
    else
    {
        assert(std::holds_alternative<std::monostate>(entry.value));
    }
}

StateRequestEntries One(StateEntry entry)
{
    StateRequestEntries entries;
    assert(entries.TryAppend(std::move(entry)));
    return entries;
}

} // namespace

int main()
{
    ConsolidatorInstance first;
    ConsolidatorInstance second;
    const std::vector<ConsolidatorInstance*> instances{&first, &second};
    std::uint64_t requestId = 1;

    const auto mainInputGainPath = StatePath::DspParameter(
        first.GetInstanceId(),
        StatePath{DeviceId::MainInputGain, ParameterId::Gain});
    const auto mainOutputGainPath = StatePath::DspParameter(
        first.GetInstanceId(),
        StatePath{DeviceId::MainOutputGain, ParameterId::Gain});
    const auto saturatorDrivePath = StatePath::DspParameter(
        first.GetInstanceId(),
        StatePath{DeviceId::Saturator, ParameterId::Drive});
    const auto compressorThresholdPath = StatePath::DspParameter(
        first.GetInstanceId(),
        StatePath{DeviceId::Compressor, ParameterId::Threshold});

    // Top-level DSP writes return the exact applied path and value.
    AssertFloatValue(
        Write(instances, first, requestId++, One(DspWrite(
            first.GetInstanceId(), DeviceId::MainInputGain, ParameterId::Gain, 6.0f))),
        mainInputGainPath,
        6.0f);
    AssertFloatValue(
        Write(instances, first, requestId++, One(DspWrite(
            first.GetInstanceId(), DeviceId::MainOutputGain, ParameterId::Gain, -3.0f))),
        mainOutputGainPath,
        -3.0f);
    AssertFloatValue(
        Write(instances, first, requestId++, One(DspWrite(
            first.GetInstanceId(), DeviceId::Saturator, ParameterId::Drive, 2.0f))),
        saturatorDrivePath,
        2.0f);
    AssertFloatValue(
        Write(instances, first, requestId++, One(DspWrite(
            first.GetInstanceId(), DeviceId::Compressor, ParameterId::Threshold, -18.0f))),
        compressorThresholdPath,
        -18.0f);

    // An unchanged write is valid and still confirms the current value.
    AssertFloatValue(
        Write(instances, first, requestId++, One(DspWrite(
            first.GetInstanceId(), DeviceId::Saturator, ParameterId::Drive, 2.0f))),
        saturatorDrivePath,
        2.0f);

    const auto firstBank0Filter3GainPath = StatePath::DspParameter(
        first.GetInstanceId(),
        StatePath{
            DeviceId::Equalizer,
            ParameterId::Gain,
            RouteNodeId::Bank0,
            RouteNodeId::Filter3});
    const auto secondBank0Filter3GainPath = StatePath::DspParameter(
        second.GetInstanceId(),
        StatePath{
            DeviceId::Equalizer,
            ParameterId::Gain,
            RouteNodeId::Bank0,
            RouteNodeId::Filter3});
    const auto secondBank2Filter3GainPath = StatePath::DspParameter(
        second.GetInstanceId(),
        StatePath{
            DeviceId::Equalizer,
            ParameterId::Gain,
            RouteNodeId::Bank2,
            RouteNodeId::Filter3});

    // Bank-specific EQ writes remain isolated before linking.
    AssertFloatValue(
        Write(instances, first, requestId++, One(DspWrite(
            first.GetInstanceId(),
            DeviceId::Equalizer,
            ParameterId::Gain,
            4.0f,
            {RouteNodeId::Bank0, RouteNodeId::Filter3}))),
        firstBank0Filter3GainPath,
        4.0f);
    AssertFloatValue(
        Read(instances, first, requestId++, firstBank0Filter3GainPath),
        firstBank0Filter3GainPath,
        4.0f);
    AssertFloatValue(
        Read(instances, second, requestId++, secondBank0Filter3GainPath),
        secondBank0Filter3GainPath,
        0.0f);

    const auto firstBank0GroupPath = StatePath::BankGroup(
        first.GetInstanceId(), BankId::Bank0);
    const auto secondBank2GroupPath = StatePath::BankGroup(
        second.GetInstanceId(), BankId::Bank2);

    // Topology writes are readable through the same state protocol.
    AssertGroupValue(
        Write(instances, first, requestId++, One(GroupWrite(
            first.GetInstanceId(), BankId::Bank0, GroupId{42}))),
        firstBank0GroupPath,
        GroupId{42});
    AssertGroupValue(
        Write(instances, second, requestId++, One(GroupWrite(
            second.GetInstanceId(), BankId::Bank2, GroupId{42}))),
        secondBank2GroupPath,
        GroupId{42});
    AssertGroupValue(
        Read(instances, first, requestId++, firstBank0GroupPath),
        firstBank0GroupPath,
        GroupId{42});
    AssertGroupValue(
        Read(instances, second, requestId++, secondBank2GroupPath),
        secondBank2GroupPath,
        GroupId{42});

    // Linking fans one EQ write out to both grouped banks.
    auto responses = Write(instances, first, requestId++, One(DspWrite(
        first.GetInstanceId(),
        DeviceId::Equalizer,
        ParameterId::Gain,
        7.0f,
        {RouteNodeId::Bank0, RouteNodeId::Filter3})));
    assert(responses.size() == 2);
    assert(responses[0].responseCount == 2);
    assert(responses[0].responseIndex == 0);
    assert(responses[1].responseIndex == 1);
    assert(!responses[0].isFinal);
    assert(responses[1].isFinal);
    assert(!responses[0].truncated && !responses[1].truncated);
    assert(responses[0].entries.size == 1);
    assert(responses[1].entries.size == 1);

    const auto& firstApplied = responses[0].entries.entries[0];
    const auto& secondApplied = responses[1].entries.entries[0];
    assert(std::get<float>(firstApplied.value) == 7.0f);
    assert(std::get<float>(secondApplied.value) == 7.0f);
    assert(
        (IsExactPath(firstApplied.path, firstBank0Filter3GainPath) &&
         IsExactPath(secondApplied.path, secondBank2Filter3GainPath)) ||
        (IsExactPath(firstApplied.path, secondBank2Filter3GainPath) &&
         IsExactPath(secondApplied.path, firstBank0Filter3GainPath)));

    AssertFloatValue(
        Read(instances, first, requestId++, firstBank0Filter3GainPath),
        firstBank0Filter3GainPath,
        7.0f);
    AssertFloatValue(
        Read(instances, second, requestId++, secondBank2Filter3GainPath),
        secondBank2Filter3GainPath,
        7.0f);

    // Unlinking updates topology and prevents subsequent fan-out.
    AssertGroupValue(
        Write(instances, second, requestId++, One(GroupWrite(
            second.GetInstanceId(), BankId::Bank2, std::nullopt))),
        secondBank2GroupPath,
        std::nullopt);
    AssertGroupValue(
        Read(instances, second, requestId++, secondBank2GroupPath),
        secondBank2GroupPath,
        std::nullopt);

    AssertFloatValue(
        Write(instances, first, requestId++, One(DspWrite(
            first.GetInstanceId(),
            DeviceId::Equalizer,
            ParameterId::Gain,
            9.0f,
            {RouteNodeId::Bank0, RouteNodeId::Filter3}))),
        firstBank0Filter3GainPath,
        9.0f);
    AssertFloatValue(
        Read(instances, second, requestId++, secondBank2Filter3GainPath),
        secondBank2Filter3GainPath,
        7.0f);

    // A local batch preserves every applied entry in one response.
    StateRequestEntries batch;
    assert(batch.TryAppend(DspWrite(
        first.GetInstanceId(), DeviceId::Saturator, ParameterId::Drive, 1.5f)));
    assert(batch.TryAppend(DspWrite(
        first.GetInstanceId(), DeviceId::Compressor, ParameterId::Threshold, -24.0f)));
    responses = Write(instances, first, requestId++, std::move(batch));
    assert(responses.size() == 1);
    assert(responses.front().entries.size == 2);
    assert(!responses.front().truncated);

    AssertFloatValue(
        Read(instances, first, requestId++, saturatorDrivePath),
        saturatorDrivePath,
        1.5f);
    AssertFloatValue(
        Read(instances, first, requestId++, compressorThresholdPath),
        compressorThresholdPath,
        -24.0f);

    // Invalid/unhandled writes still terminate the request but apply nothing.
    StatePath invalid = StatePath::Instance(first.GetInstanceId());
    invalid.field = consolidator::core::StateField::DspParameter;
    responses = Write(instances, first, requestId++, One(StateEntry{
        invalid,
        StateValue{true}}));
    assert(responses.size() == 1);
    assert(responses.front().isFinal);
    assert(responses.front().entries.size == 0);
    assert(!responses.front().truncated);

    return 0;
}