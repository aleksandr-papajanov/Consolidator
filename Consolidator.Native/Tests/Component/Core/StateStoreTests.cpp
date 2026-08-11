#include "Core/Domain/State/StateStore.h"
#include "Core/Settings/DspDeviceSettings.h"
#include "Support/StateBuilders.h"
#include "Support/TestFramework.h"

#include <variant>

using namespace consolidator;

namespace
{

const core::StateEntry& ReadOne(
    const core::StateStore& store,
    const core::StatePath& path)
{
    static core::StateResponseEntries snapshot;
    snapshot.Clear();
    store.ReadState(path, snapshot);
    EXPECT_EQ(snapshot.size, 1U);
    return snapshot.entries[0];
}

core::StateWriteStatus Apply(
    core::StateStore& store,
    core::StateEntry entry,
    core::StateResponseEntries& response)
{
    response.Clear();
    return store.WriteState(entry, response);
}

} // namespace

TEST_CASE("StateStore factory owns complete defaults and physical ranges")
{
    core::StateStore store;
    store.SetInstanceId(core::InstanceId{4});

    const auto& gain = ReadOne(store, test::DevicePath(
        core::InstanceId{4}, dsp::DeviceId::MainInputGain, dsp::ParameterId::Gain));
    EXPECT_EQ(std::get<float>(gain.value), 0.0f);
    EXPECT_EQ(std::get<float>(*gain.physicalMinimum),
              static_cast<float>(core::settings::GainDefaults::kMinGainDb));
    EXPECT_EQ(std::get<float>(*gain.physicalMaximum),
              static_cast<float>(core::settings::GainDefaults::kMaxGainDb));

    const auto& filter = ReadOne(store, test::FilterPath(
        core::InstanceId{4}, dsp::BankId::Bank0, 2, dsp::ParameterId::Frequency));
    EXPECT_EQ(std::get<float>(filter.value), 100.0f);
}

TEST_CASE("StateStore reads prefixes without leaking unrelated state")
{
    core::StateStore store;
    const core::InstanceId id{5};
    store.SetInstanceId(id);

    core::StateResponseEntries snapshot;
    store.ReadState(core::StatePath::Device(dsp::DeviceId::Compressor)
                        .WithInstance(id), snapshot);

    EXPECT_EQ(snapshot.size, 19U);
    for (std::size_t index = 0; index < snapshot.size; ++index)
    {
        EXPECT_EQ(snapshot.entries[index].path.instanceId, id);
        EXPECT_EQ(snapshot.entries[index].path.deviceId, dsp::DeviceId::Compressor);
    }
}

TEST_CASE("StateStore reports applied unchanged rejected and not handled writes")
{
    core::StateStore store;
    const core::InstanceId id{6};
    store.SetInstanceId(id);
    const auto path = test::DevicePath(
        id, dsp::DeviceId::Saturator, dsp::ParameterId::Drive);
    core::StateResponseEntries response;

    EXPECT_EQ(Apply(store, test::Write(path, 2.0f), response),
              core::StateWriteStatus::Applied);
    EXPECT_EQ(response.entries[0].status, core::StateWriteStatus::Applied);
    EXPECT_EQ(std::get<float>(response.entries[0].value), 2.0f);

    EXPECT_EQ(Apply(store, test::Write(path, 2.0f), response),
              core::StateWriteStatus::Unchanged);
    EXPECT_EQ(Apply(store, test::Write(path, true), response),
              core::StateWriteStatus::Rejected);

    auto unknown = path;
    unknown.parameterId.reset();
    EXPECT_EQ(Apply(store, test::Write(unknown, 1.0f), response),
              core::StateWriteStatus::NotHandled);
}

TEST_CASE("StateStore clamps numeric values to physical limits")
{
    core::StateStore store;
    const core::InstanceId id{8};
    store.SetInstanceId(id);
    const auto path = test::DevicePath(
        id, dsp::DeviceId::Compressor, dsp::ParameterId::Ratio);
    core::StateResponseEntries response;

    EXPECT_EQ(Apply(store, test::Write(path, 100.0f), response),
              core::StateWriteStatus::Applied);
    EXPECT_EQ(std::get<float>(response.entries[0].value),
              static_cast<float>(core::settings::CompressorDefaults::kMaxRatio));
}

TEST_CASE("StateStore writes topology and authoritative markers")
{
    core::StateStore store;
    const core::InstanceId id{10};
    store.SetInstanceId(id);
    core::StateResponseEntries response;

    EXPECT_EQ(Apply(store, test::Write(
        core::StatePath::SelectedBank(id), dsp::BankId::Bank4), response),
        core::StateWriteStatus::Applied);
    EXPECT_EQ(store.GetInstance().selectedBankId, dsp::BankId::Bank4);

    EXPECT_EQ(Apply(store, test::Write(
        core::StatePath::BankGroup(id, dsp::BankId::Bank4), core::GroupId{42}), response),
        core::StateWriteStatus::Applied);
    EXPECT_EQ(store.GetInstance().banks[4].groupId, core::GroupId{42});

    EXPECT_EQ(Apply(store, test::Write(core::StatePath::InstanceMute(id), true), response),
              core::StateWriteStatus::Applied);
    EXPECT_TRUE(store.GetInstance().audibility.mute.value);
}

TEST_CASE("StateStore validates writable path and value shape")
{
    core::StateStore store;
    const core::InstanceId id{11};
    store.SetInstanceId(id);
    const auto gain = test::DevicePath(
        id, dsp::DeviceId::MainOutputGain, dsp::ParameterId::Gain);
    const auto bypass = test::DevicePath(
        id, dsp::DeviceId::MainOutputGain, core::StateMarkerId::Bypass);

    EXPECT_TRUE(store.CanWrite(test::Write(gain, -3.0f)));
    EXPECT_FALSE(store.CanWrite(test::Write(gain, true)));
    EXPECT_TRUE(store.CanWrite(test::Write(bypass, true)));
    EXPECT_FALSE(store.CanWrite(test::Write(core::StatePath::Instance(id), id)));
}

TEST_MAIN()
