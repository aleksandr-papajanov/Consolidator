#include "Core/Domain/State/StateStore.h"
#include "Core/Routing/ProcessingStateResolver.h"
#include "Support/StateBuilders.h"
#include "Support/TestFramework.h"

#include <algorithm>

using namespace consolidator;

namespace
{

void SetMarker(core::StateStore& store, core::StatePath path, bool value)
{
    core::StateResponseEntries response;
    const auto status = store.WriteState(test::Write(path, value), response);
    EXPECT_TRUE(status == core::StateWriteStatus::Applied ||
                status == core::StateWriteStatus::Unchanged);
}

core::RuntimeResolution Resolve(const core::StateStore& store, core::InstanceId id)
{
    core::RuntimeResolution result;
    core::ProcessingStateResolver{}.Resolve(id, store, result);
    return result;
}

bool Control(
    const core::RuntimeResolution& resolution,
    const core::StatePath& target,
    core::RuntimeProperty property = core::RuntimeProperty::Active)
{
    const auto found = std::find_if(
        resolution.controls.begin(), resolution.controls.end(),
        [&target, property](const auto& update)
        {
            return update.target == target && update.property == property;
        });
    EXPECT_TRUE(found != resolution.controls.end());
    return found->value;
}

} // namespace

TEST_CASE("Processing resolver enables the complete hierarchy by default")
{
    core::StateStore store;
    const core::InstanceId id{1};
    store.SetInstanceId(id);
    const auto result = Resolve(store, id);

    EXPECT_TRUE(Control(result, core::StatePath::Device(
        dsp::DeviceId::MainInputGain).WithInstance(id)));
    EXPECT_TRUE(Control(result, core::StatePath::Device(
        dsp::DeviceId::Saturator).WithInstance(id)));
    EXPECT_TRUE(Control(result, core::StatePath::Device(
        dsp::DeviceId::Compressor).WithInstance(id)));
    EXPECT_TRUE(Control(result, core::StatePath::Device(
        dsp::DeviceId::Equalizer).WithInstance(id)));
    EXPECT_TRUE(Control(result, core::StatePath::Device(
        dsp::DeviceId::MainOutputGain).WithInstance(id)));
}

TEST_CASE("Downstream-most chain solo defines processing boundary")
{
    core::StateStore store;
    const core::InstanceId id{2};
    store.SetInstanceId(id);
    SetMarker(store, test::DevicePath(
        id, dsp::DeviceId::Saturator, core::StateMarkerId::Solo), true);
    SetMarker(store, test::DevicePath(
        id, dsp::DeviceId::Compressor, core::StateMarkerId::Solo), true);
    const auto result = Resolve(store, id);

    EXPECT_TRUE(Control(result, core::StatePath::Device(
        dsp::DeviceId::Compressor).WithInstance(id)));
    EXPECT_FALSE(Control(result, core::StatePath::Device(
        dsp::DeviceId::Equalizer).WithInstance(id)));
    EXPECT_TRUE(Control(result, core::StatePath::Device(
        dsp::DeviceId::MainOutputGain).WithInstance(id)));
}

TEST_CASE("Bank and filter solo stay inside their peer scopes")
{
    core::StateStore store;
    const core::InstanceId id{3};
    store.SetInstanceId(id);
    SetMarker(store, test::BankPath(
        id, dsp::BankId::Bank2, core::StateMarkerId::Solo), true);
    SetMarker(store, test::FilterPath(
        id, dsp::BankId::Bank2, 4, core::StateMarkerId::Solo), true);
    const auto result = Resolve(store, id);

    EXPECT_TRUE(Control(result, test::RuntimeTarget(test::BankPath(
        id, dsp::BankId::Bank2, core::StateMarkerId::Solo))));
    EXPECT_FALSE(Control(result, test::RuntimeTarget(test::BankPath(
        id, dsp::BankId::Bank1, core::StateMarkerId::Solo))));
    EXPECT_TRUE(Control(result, test::RuntimeTarget(test::FilterPath(
        id, dsp::BankId::Bank2, 4, core::StateMarkerId::Solo))));
    EXPECT_FALSE(Control(result, test::RuntimeTarget(test::FilterPath(
        id, dsp::BankId::Bank2, 3, core::StateMarkerId::Solo))));
}

TEST_CASE("Bypass wins over solo at device bank and filter scopes")
{
    core::StateStore store;
    const core::InstanceId id{4};
    store.SetInstanceId(id);
    const auto bank = test::BankPath(id, dsp::BankId::Bank3, core::StateMarkerId::Solo);
    const auto filter = test::FilterPath(id, dsp::BankId::Bank3, 1,
                                         core::StateMarkerId::Solo);
    SetMarker(store, bank, true);
    SetMarker(store, bank.WithMarker(core::StateMarkerId::Bypass), true);
    SetMarker(store, filter, true);
    SetMarker(store, filter.WithMarker(core::StateMarkerId::Bypass), true);
    const auto result = Resolve(store, id);

    EXPECT_FALSE(Control(result, test::RuntimeTarget(bank)));
    EXPECT_FALSE(Control(result, test::RuntimeTarget(filter)));
}

TEST_CASE("Detector listen is independent and keeps bypassed detector active")
{
    core::StateStore store;
    const core::InstanceId id{5};
    store.SetInstanceId(id);
    SetMarker(store, test::DevicePath(
        id, dsp::DeviceId::Compressor, core::StateMarkerId::Bypass), true);
    const auto listen = test::DetectorPath(
        id, dsp::DeviceId::Compressor, core::StateMarkerId::Listen);
    SetMarker(store, listen, true);
    const auto result = Resolve(store, id);

    EXPECT_TRUE(Control(result, test::RuntimeTarget(listen),
                        core::RuntimeProperty::Listen));
    EXPECT_TRUE(Control(result, test::RuntimeTarget(test::DetectorFilterPath(
        id, dsp::DeviceId::Compressor, 0, core::StateMarkerId::Solo))));
}

TEST_MAIN()
