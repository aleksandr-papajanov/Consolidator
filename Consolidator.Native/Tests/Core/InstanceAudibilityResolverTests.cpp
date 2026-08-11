#include <array>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <memory>
#include <thread>
#include <vector>

#include "Core/Coordinator/InstanceCoordinator.h"
#include "Core/Domain/Commands/StateProtocolCommands.h"
#include "Core/Domain/State/StateEntry.h"
#include "Core/Instance/ConsolidatorInstance.h"
#include "Dsp/Processors/Compressor/Compressor.h"
#include "Dsp/Processors/DspChain.h"
#include "Dsp/Processors/Equalizer/Filters/BellFilter.h"

namespace
{

using consolidator::core::ConsolidatorInstance;
using consolidator::core::GroupId;
using consolidator::core::InstanceId;
using consolidator::core::StateEntry;
using consolidator::core::StatePath;
using consolidator::core::StateRequestEntries;
using consolidator::core::StateValue;
using consolidator::dsp::BankId;
using consolidator::dsp::DeviceId;
using consolidator::dsp::ParameterId;

class TestInstances
{
public:
    explicit TestInstances(std::size_t count)
    {
        instances_.reserve(count);
        for (std::size_t index = 0; index < count; ++index)
        {
            instances_.push_back(std::make_unique<ConsolidatorInstance>());
            instances_.back()->Initialize();
        }
    }

    ConsolidatorInstance& operator[](std::size_t index) noexcept
    {
        return *instances_[index];
    }

    [[nodiscard]] std::size_t size() const noexcept
    {
        return instances_.size();
    }

    void ProcessAll()
    {
        constexpr std::size_t kFrameCount = 1;
        std::array<double, 2> input{0.25, 0.25};
        std::array<double, 2> reference{};
        std::array<double, 2> output{};
        std::array<double, 2> referenceOutput{};
        for (auto& instance : instances_)
        {
            instance->Process(
                input.data(),
                reference.data(),
                output.data(),
                referenceOutput.data(),
                kFrameCount);
        }
    }

private:
    std::vector<std::unique_ptr<ConsolidatorInstance>> instances_;
};

std::uint64_t NextRequestId()
{
    static std::uint64_t requestId = 1;
    return requestId++;
}

void Write(
    TestInstances& instances,
    ConsolidatorInstance& instance,
    StatePath path,
    StateValue value)
{
    StateRequestEntries entries;
    assert(entries.TryAppend(StateEntry{std::move(path), std::move(value)}));

    const auto requestId = NextRequestId();
    instance.EnqueueCommand(consolidator::core::WriteStateCommand{
        .requestId = requestId,
        .entries = entries});

    for (std::size_t attempt = 0; attempt < 200; ++attempt)
    {
        instances.ProcessAll();
        if (const auto response =
                consolidator::core::InstanceCoordinator::Get().TryDequeueResponse();
            response && response->requestId == requestId)
        {
            assert(response->entries.size == 1);
            assert(response->entries.entries[0].status.has_value());
            assert(*response->entries.entries[0].status ==
                   consolidator::core::StateWriteStatus::Applied ||
                   *response->entries.entries[0].status ==
                   consolidator::core::StateWriteStatus::Unchanged);
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }
    assert(false && "state write response timeout");
}

void Link(
    TestInstances& instances,
    ConsolidatorInstance& instance,
    BankId bank,
    GroupId group)
{
    Write(
        instances,
        instance,
        StatePath::BankGroup(instance.GetInstanceId(), bank),
        StateValue{group});
}

void SelectBank(
    TestInstances& instances,
    ConsolidatorInstance& instance,
    BankId bank)
{
    Write(
        instances,
        instance,
        StatePath::SelectedBank(instance.GetInstanceId()),
        StateValue{bank});
}

void SetOutputSolo(
    TestInstances& instances,
    ConsolidatorInstance& instance,
    bool enabled)
{
    Write(
        instances,
        instance,
        StatePath::InstanceSolo(instance.GetInstanceId()),
        StateValue{enabled});
}

void TestGlobalSoloDoesNotUseTransitiveGroups()
{
    TestInstances instances{3};
    auto& a = instances[0];
    auto& b = instances[1];
    auto& c = instances[2];

    Link(instances, a, BankId::Bank0, GroupId{100});
    Link(instances, b, BankId::Bank0, GroupId{100});
    Link(instances, b, BankId::Bank1, GroupId{101});
    Link(instances, c, BankId::Bank0, GroupId{101});

    SelectBank(instances, a, BankId::Bank0);
    SetOutputSolo(instances, a, true);

    assert(a.IsOutputEnabled());
    assert(b.IsOutputEnabled());
    assert(!c.IsOutputEnabled());
}

void TestMultipleGlobalSoloUsesUnionOfDirectGroups()
{
    TestInstances instances{5};
    auto& a = instances[0];
    auto& b = instances[1];
    auto& c = instances[2];
    auto& d = instances[3];
    auto& outsider = instances[4];

    Link(instances, a, BankId::Bank0, GroupId{100});
    Link(instances, b, BankId::Bank0, GroupId{100});
    Link(instances, c, BankId::Bank0, GroupId{200});
    Link(instances, d, BankId::Bank0, GroupId{200});
    SetOutputSolo(instances, a, true);
    SetOutputSolo(instances, c, true);

    assert(a.IsOutputEnabled());
    assert(b.IsOutputEnabled());
    assert(c.IsOutputEnabled());
    assert(d.IsOutputEnabled());
    assert(!outsider.IsOutputEnabled());
}

void TestOutputSoloStateRemainsOnSourceInstance()
{
    TestInstances instances{2};
    auto& a = instances[0];
    auto& b = instances[1];
    Link(instances, a, BankId::Bank0, GroupId{300});
    Link(instances, b, BankId::Bank3, GroupId{300});

    SetOutputSolo(instances, a, true);

    // The marker is not propagated, but audibility is group-scoped.
    assert(a.GetStateStore().GetInstance().audibility.solo.value);
    assert(!b.GetStateStore().GetInstance().audibility.solo.value);
    assert(a.IsOutputEnabled());
    assert(b.IsOutputEnabled());
}

void TestSelectedBankChangeMovesGlobalSoloScope()
{
    TestInstances instances{3};
    auto& a = instances[0];
    auto& b = instances[1];
    auto& c = instances[2];

    Link(instances, a, BankId::Bank0, GroupId{10});
    Link(instances, b, BankId::Bank0, GroupId{10});
    Link(instances, a, BankId::Bank1, GroupId{20});
    Link(instances, c, BankId::Bank0, GroupId{20});
    SetOutputSolo(instances, a, true);

    SelectBank(instances, a, BankId::Bank0);
    assert(a.IsOutputEnabled());
    assert(b.IsOutputEnabled());
    assert(!c.IsOutputEnabled());

    SelectBank(instances, a, BankId::Bank1);
    assert(a.IsOutputEnabled());
    assert(!b.IsOutputEnabled());
    assert(c.IsOutputEnabled());
}

void TestUnlinkImmediatelyUpdatesGlobalSoloScope()
{
    TestInstances instances{2};
    auto& a = instances[0];
    auto& b = instances[1];
    Link(instances, a, BankId::Bank0, GroupId{30});
    Link(instances, b, BankId::Bank0, GroupId{30});
    SetOutputSolo(instances, a, true);
    assert(b.IsOutputEnabled());

    Write(
        instances,
        b,
        StatePath::BankGroup(b.GetInstanceId(), BankId::Bank0),
        StateValue{std::monostate{}});
    assert(a.IsOutputEnabled());
    assert(!b.IsOutputEnabled());
}

void TestDisablingLastGlobalSoloUnmutesEveryone()
{
    TestInstances instances{2};
    auto& a = instances[0];
    auto& outsider = instances[1];
    SetOutputSolo(instances, a, true);
    assert(!outsider.IsOutputEnabled());

    SetOutputSolo(instances, a, false);
    assert(a.IsOutputEnabled());
    assert(outsider.IsOutputEnabled());
}

void TestOutputDisabledProducesSilence()
{
    TestInstances instances{2};
    auto& source = instances[0];
    auto& muted = instances[1];
    SetOutputSolo(instances, source, true);
    assert(!muted.IsOutputEnabled());

    std::array<double, 2> input{0.25, 0.25};
    std::array<double, 2> reference{};
    std::array<double, 2> output{1.0, 1.0};
    std::array<double, 2> referenceOutput{};
    muted.Process(
        input.data(), reference.data(), output.data(), referenceOutput.data(), 1);
    assert(output[0] == 0.0);
    assert(output[1] == 0.0);
}

void TestOutputEnabledPassesAudio()
{
    ConsolidatorInstance instance;
    instance.Initialize();

    std::array<double, 2> input{0.25, 0.25};
    std::array<double, 2> reference{};
    std::array<double, 2> output{};
    std::array<double, 2> referenceOutput{};
    instance.Process(
        input.data(), reference.data(), output.data(), referenceOutput.data(), 1);

    assert(output[0] != 0.0 || output[1] != 0.0);
}

void TestCompressorNormalProcessingPreservesStereoChannels()
{
    consolidator::dsp::Compressor compressor;
    compressor.Prepare(48000.0, 2);
    std::array<double, 16> input{};
    std::array<double, 16> output{};
    for (std::size_t frame = 0; frame < 8; ++frame)
    {
        input[frame * 2] = 0.25;
        input[frame * 2 + 1] = -0.5;
    }
    compressor.Process(input.data(), output.data(), 8, 2);

    bool channelsDiffer = false;
    for (std::size_t frame = 0; frame < 8; ++frame)
    {
        channelsDiffer = channelsDiffer ||
            output[frame * 2] != output[frame * 2 + 1];
    }
    assert(channelsDiffer);
}

} // namespace

int main()
{
    TestGlobalSoloDoesNotUseTransitiveGroups();
    TestMultipleGlobalSoloUsesUnionOfDirectGroups();
    TestOutputSoloStateRemainsOnSourceInstance();
    TestSelectedBankChangeMovesGlobalSoloScope();
    TestUnlinkImmediatelyUpdatesGlobalSoloScope();
    TestDisablingLastGlobalSoloUnmutesEveryone();
    TestOutputDisabledProducesSilence();
    TestOutputEnabledPassesAudio();
    TestCompressorNormalProcessingPreservesStereoChannels();
    return 0;
}
