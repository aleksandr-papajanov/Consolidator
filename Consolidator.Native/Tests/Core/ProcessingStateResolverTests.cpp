#include <cassert>
#include <cstddef>

#include "Core/Domain/State/StateStore.h"
#include "Core/Routing/ProcessingStateResolver.h"

namespace
{

using namespace consolidator::core;
using namespace consolidator::dsp;

StatePath DeviceParameter(
    InstanceId instanceId,
    DeviceId deviceId,
    ParameterId parameterId)
{
    return StatePath::DspParameter(
        instanceId,
        StatePath::Device(deviceId)
            .WithParameter(parameterId));
}

StatePath BankParameter(
    InstanceId instanceId,
    BankId bankId,
    ParameterId parameterId)
{
    const auto bankNode = static_cast<RouteNodeId>(
        static_cast<std::uint8_t>(RouteNodeId::Bank0) +
        detail::ToIndex(bankId));

    return StatePath::DspParameter(
        instanceId,
        StatePath::Device(DeviceId::Equalizer)
            .WithNode(bankNode)
            .WithParameter(parameterId));
}

StatePath FilterParameter(
    InstanceId instanceId,
    BankId bankId,
    std::size_t filterIndex,
    ParameterId parameterId)
{
    const auto bankNode = static_cast<RouteNodeId>(
        static_cast<std::uint8_t>(RouteNodeId::Bank0) +
        detail::ToIndex(bankId));

    const auto filterNode = static_cast<RouteNodeId>(
        static_cast<std::uint8_t>(RouteNodeId::Filter1) +
        filterIndex);

    return StatePath::DspParameter(
        instanceId,
        StatePath::Device(DeviceId::Equalizer)
            .WithNode(bankNode)
            .WithNode(filterNode)
            .WithParameter(parameterId));
}

StatePath DetectorParameter(
    InstanceId instanceId,
    DeviceId deviceId,
    ParameterId parameterId)
{
    return StatePath::DspParameter(
        instanceId,
        StatePath::Device(deviceId)
            .WithNode(RouteNodeId::Detector)
            .WithParameter(parameterId));
}

StatePath DetectorFilterParameter(
    InstanceId instanceId,
    DeviceId deviceId,
    std::size_t filterIndex,
    ParameterId parameterId)
{
    const auto filterNode = static_cast<RouteNodeId>(
        static_cast<std::uint8_t>(RouteNodeId::Filter1) +
        filterIndex);

    return StatePath::DspParameter(
        instanceId,
        StatePath::Device(deviceId)
            .WithNode(RouteNodeId::Detector)
            .WithNode(filterNode)
            .WithParameter(parameterId));
}

void WriteBool(
    StateStore& store,
    StatePath path,
    bool value)
{
    StateResponseEntries applied;

    const auto status = store.WriteState(
        StateEntry{
            std::move(path),
            StateValue{value}},
        applied);

    assert(
        status == StateWriteStatus::Applied ||
        status == StateWriteStatus::Unchanged);
}

bool ReadControl(
    const RuntimeResolution& resolution,
    const StatePath& target,
    RuntimeProperty property)
{
    for (const auto& update : resolution.controls)
    {
        if (update.target == target &&
            update.property == property)
        {
            return update.value;
        }
    }

    assert(false && "runtime control not found");
    return false;
}

bool Active(
    const RuntimeResolution& resolution,
    const StatePath& target)
{
    return ReadControl(
        resolution,
        target,
        RuntimeProperty::Active);
}

RuntimeResolution Resolve(
    InstanceId instanceId,
    const StateStore& store)
{
    RuntimeResolution result;

    ProcessingStateResolver{}.Resolve(
        instanceId,
        store,
        result);

    return result;
}

void TestEverythingActiveByDefault()
{
    StateStore store;
    const InstanceId id{1};
    store.SetInstanceId(id);

    const auto result = Resolve(id, store);

    assert(Active(
        result,
        StatePath::Device(DeviceId::MainInputGain)
            .WithInstance(id)));

    assert(Active(
        result,
        StatePath::Device(DeviceId::Saturator)
            .WithInstance(id)));

    assert(Active(
        result,
        StatePath::Device(DeviceId::Compressor)
            .WithInstance(id)));

    assert(Active(
        result,
        StatePath::Device(DeviceId::Equalizer)
            .WithInstance(id)));

    assert(Active(
        result,
        StatePath::Device(DeviceId::MainOutputGain)
            .WithInstance(id)));
}

void TestDeviceBypassOnlyDisablesThatDevice()
{
    StateStore store;
    const InstanceId id{2};
    store.SetInstanceId(id);

    WriteBool(
        store,
        DeviceParameter(
            id,
            DeviceId::Saturator,
            ParameterId::Bypass),
        true);

    const auto result = Resolve(id, store);

    assert(Active(
        result,
        StatePath::Device(DeviceId::MainInputGain)
            .WithInstance(id)));

    assert(!Active(
        result,
        StatePath::Device(DeviceId::Saturator)
            .WithInstance(id)));

    assert(Active(
        result,
        StatePath::Device(DeviceId::Compressor)
            .WithInstance(id)));

    assert(Active(
        result,
        StatePath::Device(DeviceId::Equalizer)
            .WithInstance(id)));

    assert(Active(
        result,
        StatePath::Device(DeviceId::MainOutputGain)
            .WithInstance(id)));
}

void TestCompressorSoloPreservesUpstreamAndOutputGain()
{
    StateStore store;
    const InstanceId id{3};
    store.SetInstanceId(id);

    WriteBool(
        store,
        DeviceParameter(
            id,
            DeviceId::Compressor,
            ParameterId::Solo),
        true);

    const auto result = Resolve(id, store);

    assert(Active(
        result,
        StatePath::Device(DeviceId::MainInputGain)
            .WithInstance(id)));

    assert(Active(
        result,
        StatePath::Device(DeviceId::Saturator)
            .WithInstance(id)));

    assert(Active(
        result,
        StatePath::Device(DeviceId::Compressor)
            .WithInstance(id)));

    assert(!Active(
        result,
        StatePath::Device(DeviceId::Equalizer)
            .WithInstance(id)));

    // Mandatory post stage.
    assert(Active(
        result,
        StatePath::Device(DeviceId::MainOutputGain)
            .WithInstance(id)));
}

void TestInputGainSoloDisablesEveryDownstreamProcessorExceptOutput()
{
    StateStore store;
    const InstanceId id{4};
    store.SetInstanceId(id);

    WriteBool(
        store,
        DeviceParameter(
            id,
            DeviceId::MainInputGain,
            ParameterId::Solo),
        true);

    const auto result = Resolve(id, store);

    assert(Active(
        result,
        StatePath::Device(DeviceId::MainInputGain)
            .WithInstance(id)));

    assert(!Active(
        result,
        StatePath::Device(DeviceId::Saturator)
            .WithInstance(id)));

    assert(!Active(
        result,
        StatePath::Device(DeviceId::Compressor)
            .WithInstance(id)));

    assert(!Active(
        result,
        StatePath::Device(DeviceId::Equalizer)
            .WithInstance(id)));

    assert(Active(
        result,
        StatePath::Device(DeviceId::MainOutputGain)
            .WithInstance(id)));
}

void TestLastSequentialSoloWins()
{
    StateStore store;
    const InstanceId id{5};
    store.SetInstanceId(id);

    WriteBool(
        store,
        DeviceParameter(
            id,
            DeviceId::MainInputGain,
            ParameterId::Solo),
        true);

    WriteBool(
        store,
        DeviceParameter(
            id,
            DeviceId::Compressor,
            ParameterId::Solo),
        true);

    const auto result = Resolve(id, store);

    assert(Active(
        result,
        StatePath::Device(DeviceId::MainInputGain)
            .WithInstance(id)));

    assert(Active(
        result,
        StatePath::Device(DeviceId::Saturator)
            .WithInstance(id)));

    assert(Active(
        result,
        StatePath::Device(DeviceId::Compressor)
            .WithInstance(id)));

    assert(!Active(
        result,
        StatePath::Device(DeviceId::Equalizer)
            .WithInstance(id)));
}

void TestOutputSoloDoesNotAffectLocalProcessing()
{
    StateStore store;
    const InstanceId id{6};
    store.SetInstanceId(id);

    WriteBool(
        store,
        DeviceParameter(
            id,
            DeviceId::MainOutputGain,
            ParameterId::Solo),
        true);

    const auto result = Resolve(id, store);

    assert(Active(
        result,
        StatePath::Device(DeviceId::MainInputGain)
            .WithInstance(id)));

    assert(Active(
        result,
        StatePath::Device(DeviceId::Saturator)
            .WithInstance(id)));

    assert(Active(
        result,
        StatePath::Device(DeviceId::Compressor)
            .WithInstance(id)));

    assert(Active(
        result,
        StatePath::Device(DeviceId::Equalizer)
            .WithInstance(id)));

    assert(Active(
        result,
        StatePath::Device(DeviceId::MainOutputGain)
            .WithInstance(id)));
}

void TestBankSoloIsScopedToBanks()
{
    StateStore store;
    const InstanceId id{7};
    store.SetInstanceId(id);

    WriteBool(
        store,
        BankParameter(
            id,
            BankId::Bank2,
            ParameterId::Solo),
        true);

    const auto result = Resolve(id, store);

    for (std::size_t index = 0;
         index < InstanceState::kBankCount;
         ++index)
    {
        const auto bankId =
            static_cast<BankId>(index);

        const auto target =
            BankParameter(
                id,
                bankId,
                ParameterId::Solo);

        auto runtimeTarget = target;
        runtimeTarget.parameterId.reset();

        assert(
            Active(result, runtimeTarget) ==
            (bankId == BankId::Bank2));
    }

    // Bank solo must NOT disable chain devices.
    assert(Active(
        result,
        StatePath::Device(DeviceId::Compressor)
            .WithInstance(id)));

    assert(Active(
        result,
        StatePath::Device(DeviceId::Equalizer)
            .WithInstance(id)));
}

void TestMultipleBankSoloKeepsAllSoloBanks()
{
    StateStore store;
    const InstanceId id{8};
    store.SetInstanceId(id);

    WriteBool(
        store,
        BankParameter(
            id,
            BankId::Bank1,
            ParameterId::Solo),
        true);

    WriteBool(
        store,
        BankParameter(
            id,
            BankId::Bank4,
            ParameterId::Solo),
        true);

    const auto result = Resolve(id, store);

    for (std::size_t index = 0;
         index < InstanceState::kBankCount;
         ++index)
    {
        const auto bankId =
            static_cast<BankId>(index);

        auto target =
            BankParameter(
                id,
                bankId,
                ParameterId::Solo);

        target.parameterId.reset();

        const bool expected =
            bankId == BankId::Bank1 ||
            bankId == BankId::Bank4;

        assert(Active(result, target) == expected);
    }
}

void TestBankBypassWinsOverSolo()
{
    StateStore store;
    const InstanceId id{9};
    store.SetInstanceId(id);

    WriteBool(
        store,
        BankParameter(
            id,
            BankId::Bank3,
            ParameterId::Solo),
        true);

    WriteBool(
        store,
        BankParameter(
            id,
            BankId::Bank3,
            ParameterId::Bypass),
        true);

    const auto result = Resolve(id, store);

    auto bank = BankParameter(
        id,
        BankId::Bank3,
        ParameterId::Solo);

    bank.parameterId.reset();

    assert(!Active(result, bank));
}

void TestFilterSoloIsScopedToOneBank()
{
    StateStore store;
    const InstanceId id{10};
    store.SetInstanceId(id);

    WriteBool(
        store,
        FilterParameter(
            id,
            BankId::Bank2,
            3,
            ParameterId::Solo),
        true);

    const auto result = Resolve(id, store);

    for (std::size_t filter = 0; filter < 7; ++filter)
    {
        auto target = FilterParameter(
            id,
            BankId::Bank2,
            filter,
            ParameterId::Solo);

        target.parameterId.reset();

        assert(Active(result, target) == (filter == 3));
    }

    // Another bank is unaffected.
    for (std::size_t filter = 0; filter < 7; ++filter)
    {
        auto target = FilterParameter(
            id,
            BankId::Bank1,
            filter,
            ParameterId::Solo);

        target.parameterId.reset();

        assert(Active(result, target));
    }
}

void TestMultipleFilterSoloKeepsBothFilters()
{
    StateStore store;
    const InstanceId id{11};
    store.SetInstanceId(id);

    WriteBool(
        store,
        FilterParameter(
            id,
            BankId::Bank0,
            1,
            ParameterId::Solo),
        true);

    WriteBool(
        store,
        FilterParameter(
            id,
            BankId::Bank0,
            5,
            ParameterId::Solo),
        true);

    const auto result = Resolve(id, store);

    for (std::size_t filter = 0; filter < 7; ++filter)
    {
        auto target = FilterParameter(
            id,
            BankId::Bank0,
            filter,
            ParameterId::Solo);

        target.parameterId.reset();

        assert(
            Active(result, target) ==
            (filter == 1 || filter == 5));
    }
}

void TestFilterBypassWinsOverFilterSolo()
{
    StateStore store;
    const InstanceId id{12};
    store.SetInstanceId(id);

    WriteBool(
        store,
        FilterParameter(
            id,
            BankId::Bank0,
            2,
            ParameterId::Solo),
        true);

    WriteBool(
        store,
        FilterParameter(
            id,
            BankId::Bank0,
            2,
            ParameterId::Bypass),
        true);

    const auto result = Resolve(id, store);

    auto target = FilterParameter(
        id,
        BankId::Bank0,
        2,
        ParameterId::Solo);

    target.parameterId.reset();

    assert(!Active(result, target));
}

void TestEqualizerBypassDisablesBanksAndFilters()
{
    StateStore store;
    const InstanceId id{13};
    store.SetInstanceId(id);

    WriteBool(
        store,
        DeviceParameter(
            id,
            DeviceId::Equalizer,
            ParameterId::Bypass),
        true);

    const auto result = Resolve(id, store);

    for (std::size_t bank = 0;
         bank < InstanceState::kBankCount;
         ++bank)
    {
        auto bankTarget = BankParameter(
            id,
            static_cast<BankId>(bank),
            ParameterId::Bypass);

        bankTarget.parameterId.reset();

        assert(!Active(result, bankTarget));

        for (std::size_t filter = 0; filter < 7; ++filter)
        {
            auto filterTarget = FilterParameter(
                id,
                static_cast<BankId>(bank),
                filter,
                ParameterId::Bypass);

            filterTarget.parameterId.reset();

            assert(!Active(result, filterTarget));
        }
    }
}

void TestDetectorFilterSoloIsLocal()
{
    StateStore store;
    const InstanceId id{14};
    store.SetInstanceId(id);

    WriteBool(
        store,
        DetectorFilterParameter(
            id,
            DeviceId::Compressor,
            1,
            ParameterId::Solo),
        true);

    const auto result = Resolve(id, store);

    auto first = DetectorFilterParameter(
        id,
        DeviceId::Compressor,
        0,
        ParameterId::Solo);

    auto second = DetectorFilterParameter(
        id,
        DeviceId::Compressor,
        1,
        ParameterId::Solo);

    first.parameterId.reset();
    second.parameterId.reset();

    assert(!Active(result, first));
    assert(Active(result, second));
}

void TestDetectorListenProducesMonitoringControl()
{
    StateStore store;
    const InstanceId id{15};
    store.SetInstanceId(id);

    WriteBool(
        store,
        DetectorParameter(
            id,
            DeviceId::Compressor,
            ParameterId::Listen),
        true);

    const auto result = Resolve(id, store);

    auto target =
        StatePath::Device(DeviceId::Compressor)
            .WithNode(RouteNodeId::Detector);

    target.instanceId = id;

    assert(ReadControl(
        result,
        target,
        RuntimeProperty::Listen));
}

void TestDisabledParentDetectorStillRunsWhenListening()
{
    StateStore store;
    const InstanceId id{16};
    store.SetInstanceId(id);

    WriteBool(
        store,
        DeviceParameter(
            id,
            DeviceId::Compressor,
            ParameterId::Bypass),
        true);

    WriteBool(
        store,
        DetectorParameter(
            id,
            DeviceId::Compressor,
            ParameterId::Listen),
        true);

    const auto result = Resolve(id, store);

    for (std::size_t filter = 0; filter < 2; ++filter)
    {
        auto target = DetectorFilterParameter(
            id,
            DeviceId::Compressor,
            filter,
            ParameterId::Solo);

        target.parameterId.reset();

        assert(Active(result, target));
    }
}

} // namespace

int main()
{
    TestEverythingActiveByDefault();
    TestDeviceBypassOnlyDisablesThatDevice();

    TestCompressorSoloPreservesUpstreamAndOutputGain();
    TestInputGainSoloDisablesEveryDownstreamProcessorExceptOutput();
    TestLastSequentialSoloWins();
    TestOutputSoloDoesNotAffectLocalProcessing();

    TestBankSoloIsScopedToBanks();
    TestMultipleBankSoloKeepsAllSoloBanks();
    TestBankBypassWinsOverSolo();

    TestFilterSoloIsScopedToOneBank();
    TestMultipleFilterSoloKeepsBothFilters();
    TestFilterBypassWinsOverFilterSolo();

    TestEqualizerBypassDisablesBanksAndFilters();

    TestDetectorFilterSoloIsLocal();
    TestDetectorListenProducesMonitoringControl();
    TestDisabledParentDetectorStillRunsWhenListening();

    return 0;
}