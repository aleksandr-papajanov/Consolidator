#include "Core/Domain/Commands/StateProtocolCommands.h"
#include "Core/Coordinator/InstanceCoordinator.h"
#include "Core/Instance/ConsolidatorInstance.h"
#include "Core/Domain/State/StateProtocol.h"

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
using consolidator::core::StateWriteStatus;
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
    source.HandleStateCommand(StateCommand{
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

    source.HandleStateCommand(StateCommand{
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
    const StateEntry* entry = nullptr;
    for (std::size_t index = 0; index < responses.size(); ++index)
    {
        for (std::size_t entryIndex = 0;
             entryIndex < responses[index].entries.size;
             ++entryIndex)
        {
            if (IsExactPath(
                    responses[index].entries.entries[entryIndex].path,
                    expectedPath))
            {
                entry = &responses[index].entries.entries[entryIndex];
                break;
            }
        }
        if (entry != nullptr)
        {
            break;
        }
    }
    assert(entry != nullptr);
    if (expectedValue)
    {
        assert(std::holds_alternative<GroupId>(entry->value));
        assert(std::get<GroupId>(entry->value) == *expectedValue);
    }
    else
    {
        assert(std::holds_alternative<std::monostate>(entry->value));
    }
}

void AssertConstraintEntry(
    const StateResponse& response,
    const StatePath& path)
{
    for (std::size_t index = 0; index < response.entries.size; ++index)
    {
        const auto& entry = response.entries.entries[index];
        if (IsExactPath(entry.path, path))
        {
            assert(entry.minimum.has_value());
            assert(entry.maximum.has_value());
            return;
        }
    }
    assert(false && "missing constraint entry");
}

std::size_t CountEntries(
    const StateResponse& response,
    const StatePath& path)
{
    std::size_t count = 0;
    for (std::size_t index = 0; index < response.entries.size; ++index)
    {
        if (IsExactPath(response.entries.entries[index].path, path))
        {
            ++count;
        }
    }
    return count;
}

void AssertFloatEntry(
    const StateResponse& response,
    const StatePath& path,
    float expectedValue)
{
    const StateEntry* matchingEntry = nullptr;
    for (std::size_t index = 0; index < response.entries.size; ++index)
    {
        if (IsExactPath(response.entries.entries[index].path, path))
        {
            matchingEntry = &response.entries.entries[index];
            break;
        }
    }
    assert(matchingEntry != nullptr);
    assert(std::holds_alternative<float>(matchingEntry->value));
    assert(std::get<float>(matchingEntry->value) == expectedValue);
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
    ConsolidatorInstance third;
    ConsolidatorInstance fourth;
    first.Initialize();
    second.Initialize();
    third.Initialize();
    fourth.Initialize();
    const std::vector<ConsolidatorInstance*> instances{
        &first,
        &second,
        &third,
        &fourth};
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

    // A local batch preserves every applied entry in one response before any
    // group topology connects the instances.
    StateRequestEntries batch;
    assert(batch.TryAppend(DspWrite(
        first.GetInstanceId(), DeviceId::Saturator, ParameterId::Drive, 1.5f)));
    assert(batch.TryAppend(DspWrite(
        first.GetInstanceId(), DeviceId::Compressor, ParameterId::Threshold, -24.0f)));
    auto responses = Write(instances, first, requestId++, std::move(batch));
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
    responses = Write(instances, first, requestId++, One(DspWrite(
        first.GetInstanceId(),
        DeviceId::Equalizer,
        ParameterId::Gain,
        7.0f,
        {RouteNodeId::Bank0, RouteNodeId::Filter3})));
    assert(responses.size() == 1);
    assert(responses.front().responseCount == 1);
    assert(responses.front().responseIndex == 0);
    assert(responses.front().isFinal);
    assert(!responses.front().truncated);
    assert(responses.front().entries.size == 2);

    const auto& firstApplied = responses.front().entries.entries[0];
    const auto& secondApplied = responses.front().entries.entries[1];
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

    // Constraint dependencies traverse an overlapping chain of groups:
    // first/Bank1 -- second/Bank0 -- second/Bank1 -- third/Bank0
    // -- third/Bank1 -- fourth/Bank0. One write must refresh all four
    // instance-owned parameter entries in one response.
    const auto chainGroups = {
        std::pair{&first, BankId::Bank1},
        std::pair{&second, BankId::Bank0},
        std::pair{&second, BankId::Bank1},
        std::pair{&third, BankId::Bank0},
        std::pair{&third, BankId::Bank1},
        std::pair{&fourth, BankId::Bank0}};
    const auto chainGroupIds = {
        GroupId{100}, GroupId{100}, GroupId{101},
        GroupId{101}, GroupId{102}, GroupId{102}};
    auto groupId = chainGroupIds.begin();
    for (const auto& [instance, bank] : chainGroups)
    {
        const auto currentGroupId = *groupId++;
        AssertGroupValue(
            Write(instances, *instance, requestId++, One(GroupWrite(
                instance->GetInstanceId(), bank, currentGroupId))),
            StatePath::BankGroup(instance->GetInstanceId(), bank),
            currentGroupId);
    }

    const auto chainRatioPath = [](const ConsolidatorInstance& instance)
    {
        return StatePath::DspParameter(
            instance.GetInstanceId(),
            StatePath{DeviceId::Compressor, ParameterId::Ratio});
    };
    auto chainResponse = Write(instances, second, requestId++, One(DspWrite(
        second.GetInstanceId(),
        DeviceId::Compressor,
        ParameterId::Ratio,
        2.0f)));
    assert(chainResponse.size() == 1);
    assert(chainResponse.front().responseCount == 1);
    assert(chainResponse.front().entries.size == 4);
    AssertConstraintEntry(chainResponse.front(), chainRatioPath(first));
    AssertConstraintEntry(chainResponse.front(), chainRatioPath(second));
    AssertConstraintEntry(chainResponse.front(), chainRatioPath(third));
    AssertConstraintEntry(chainResponse.front(), chainRatioPath(fourth));
    AssertFloatEntry(
        chainResponse.front(),
        chainRatioPath(first),
        2.0f);
    AssertFloatEntry(
        chainResponse.front(),
        chainRatioPath(second),
        2.0f);
    AssertFloatEntry(
        chainResponse.front(),
        chainRatioPath(third),
        1.0f);
    AssertFloatEntry(
        chainResponse.front(),
        chainRatioPath(fourth),
        1.0f);
    assert(CountEntries(
               chainResponse.front(),
               chainRatioPath(first)) == 1);
    assert(CountEntries(
               chainResponse.front(),
               chainRatioPath(second)) == 1);
    assert(CountEntries(
               chainResponse.front(),
               chainRatioPath(third)) == 1);
    assert(CountEntries(
               chainResponse.front(),
               chainRatioPath(fourth)) == 1);

    // The write itself is NOT transitive. second is editing Bank0, therefore
    // only Group100 (first/Bank1 + second/Bank0) receives the new value.
    // third and fourth are present in the response only because their
    // effective constraints depend transitively on the changed value.
    AssertFloatValue(
        Read(instances, first, requestId++, chainRatioPath(first)),
        chainRatioPath(first),
        2.0f);
    AssertFloatValue(
        Read(instances, second, requestId++, chainRatioPath(second)),
        chainRatioPath(second),
        2.0f);
    AssertFloatValue(
        Read(instances, third, requestId++, chainRatioPath(third)),
        chainRatioPath(third),
        1.0f);
    AssertFloatValue(
        Read(instances, fourth, requestId++, chainRatioPath(fourth)),
        chainRatioPath(fourth),
        1.0f);

    // Invalid/unhandled writes still terminate the request but apply nothing.
    StatePath invalid = StatePath::Instance(first.GetInstanceId());
    invalid.field = consolidator::core::StateField::DspParameter;
    responses = Write(instances, first, requestId++, One(StateEntry{
        invalid,
        StateValue{true}}));
    assert(responses.size() == 1);
    assert(responses.front().isFinal);
    assert(responses.front().entries.size == 1);
    assert(!responses.front().truncated);
    assert(responses.front().entries.entries[0].status ==
           StateWriteStatus::Rejected);

    return 0;
}
