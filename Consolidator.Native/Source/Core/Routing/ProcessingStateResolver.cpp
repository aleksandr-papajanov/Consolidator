#include "Core/Routing/ProcessingStateResolver.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace consolidator::core
{

namespace
{

enum class ChainSlot : std::size_t
{
    InputGain,
    Saturator,
    Compressor,
    Equalizer,
    OutputGain,
    Count
};

constexpr std::size_t ToIndex(ChainSlot slot) noexcept
{
    return static_cast<std::size_t>(slot);
}

constexpr std::size_t kBankCount = 7;

StatePath DevicePath(InstanceId instanceId, dsp::DeviceId deviceId)
{
    auto path = StatePath::Device(deviceId);
    path.instanceId = instanceId;
    return path;
}

StatePath BankPath(InstanceId instanceId, dsp::BankId bankId)
{
    auto path = StatePath::Device(dsp::DeviceId::Equalizer).WithNode(
        static_cast<dsp::RouteNodeId>(
            static_cast<std::uint8_t>(dsp::RouteNodeId::Bank0) +
            dsp::detail::ToIndex(bankId)));
    path.instanceId = instanceId;
    return path;
}

StatePath FilterPath(
    InstanceId instanceId,
    dsp::BankId bankId,
    std::size_t filterIndex)
{
    auto path = BankPath(instanceId, bankId).WithNode(
        static_cast<dsp::RouteNodeId>(
            static_cast<std::uint8_t>(dsp::RouteNodeId::Filter1) +
            static_cast<std::uint8_t>(filterIndex)));
    return path;
}

StatePath DetectorFilterPath(
    InstanceId instanceId,
    dsp::DeviceId deviceId,
    std::size_t filterIndex)
{
    auto path = StatePath::Device(deviceId)
        .WithNode(dsp::RouteNodeId::Detector)
        .WithNode(static_cast<dsp::RouteNodeId>(
            static_cast<std::uint8_t>(dsp::RouteNodeId::Filter1) +
            static_cast<std::uint8_t>(filterIndex)));
    path.instanceId = instanceId;
    return path;
}

} // namespace

void ProcessingStateResolver::Resolve(
    InstanceId instanceId,
    const StateStore& stateStore,
    RuntimeResolution& resolution) const
{
    resolution.controls.clear();
    bool saturatorActive = false;
    bool compressorActive = false;
    const bool equalizerActive = ResolveChain(
        instanceId,
        stateStore,
        resolution.controls,
        saturatorActive,
        compressorActive);
    ResolveEqualizer(
        instanceId,
        stateStore,
        equalizerActive,
        resolution.controls);
    ResolveDetectorFilters(
        instanceId,
        stateStore,
        saturatorActive,
        compressorActive,
        resolution.controls);
    ResolveMonitoring(instanceId, stateStore, resolution.controls);
}

bool ProcessingStateResolver::ResolveChain(
    InstanceId instanceId,
    const StateStore& stateStore,
    std::vector<RuntimeControlUpdate>& updates,
    bool& saturatorActive,
    bool& compressorActive) const
{
    const auto& chain = stateStore.GetChain();
    const std::array<bool, ToIndex(ChainSlot::Count)> solo{
        false,
        chain.saturator.solo.value,
        chain.compressor.solo.value,
        chain.equalizer.solo.value,
        false};
    const std::array<bool, ToIndex(ChainSlot::Count)> bypass{
        chain.inputGain.bypass.value,
        chain.saturator.bypass.value,
        chain.compressor.bypass.value,
        chain.equalizer.bypass.value,
        chain.outputGain.bypass.value};

    const auto noSolo = ToIndex(ChainSlot::Count);
    std::size_t lastSolo = noSolo;
    for (std::size_t index = 0; index < noSolo; ++index)
    {
        if (solo[index])
        {
            lastSolo = index;
        }
    }

    const auto isAllowed = [lastSolo, noSolo](std::size_t index)
    {
        return lastSolo == noSolo || index <= lastSolo;
    };
    AppendActiveUpdate(
        DevicePath(instanceId, dsp::DeviceId::MainInputGain),
        !bypass[ToIndex(ChainSlot::InputGain)] &&
            isAllowed(ToIndex(ChainSlot::InputGain)),
        updates);
    saturatorActive =
        !bypass[ToIndex(ChainSlot::Saturator)] &&
        isAllowed(ToIndex(ChainSlot::Saturator));
    AppendActiveUpdate(
        DevicePath(instanceId, dsp::DeviceId::Saturator),
        saturatorActive,
        updates);
    compressorActive =
        !bypass[ToIndex(ChainSlot::Compressor)] &&
        isAllowed(ToIndex(ChainSlot::Compressor));
    AppendActiveUpdate(
        DevicePath(instanceId, dsp::DeviceId::Compressor),
        compressorActive,
        updates);

    const bool equalizerActive =
        !bypass[ToIndex(ChainSlot::Equalizer)] &&
        isAllowed(ToIndex(ChainSlot::Equalizer));
    AppendActiveUpdate(
        DevicePath(instanceId, dsp::DeviceId::Equalizer),
        equalizerActive,
        updates);
    AppendActiveUpdate(
        DevicePath(instanceId, dsp::DeviceId::MainOutputGain),
        !bypass[ToIndex(ChainSlot::OutputGain)],
        updates);
    return equalizerActive;
}

void ProcessingStateResolver::ResolveEqualizer(
    InstanceId instanceId,
    const StateStore& stateStore,
    bool equalizerActive,
    std::vector<RuntimeControlUpdate>& updates) const
{
    const auto& banks = stateStore.GetChain().equalizers;
    bool hasSolo = false;
    for (const auto& bank : banks)
    {
        hasSolo = hasSolo || bank.solo.value;
    }

    for (std::size_t bankIndex = 0; bankIndex < kBankCount; ++bankIndex)
    {
        const auto& bank = banks[bankIndex];
        const bool bankActive = equalizerActive &&
            !bank.bypass.value &&
            (!hasSolo || bank.solo.value);
        AppendActiveUpdate(
            BankPath(instanceId, static_cast<dsp::BankId>(bankIndex)),
            bankActive,
            updates);
        ResolveBankFilters(
            instanceId,
            static_cast<dsp::BankId>(bankIndex),
            bank,
            bankActive,
            updates);
    }
}

void ProcessingStateResolver::ResolveBankFilters(
    InstanceId instanceId,
    dsp::BankId bankId,
    const dsp::EqualizerBankState& bank,
    bool bankActive,
    std::vector<RuntimeControlUpdate>& updates) const
{
    bool hasSolo = false;
    for (const auto& filter : bank.filters)
    {
        hasSolo = hasSolo || filter.solo.value;
    }
    for (std::size_t filterIndex = 0; filterIndex < bank.filters.size(); ++filterIndex)
    {
        const auto& filter = bank.filters[filterIndex];
        AppendActiveUpdate(
            FilterPath(instanceId, bankId, filterIndex),
            bankActive && !filter.bypass.value &&
                (!hasSolo || filter.solo.value),
            updates);
    }
}

void ProcessingStateResolver::ResolveDetectorFilters(
    InstanceId instanceId,
    const StateStore& stateStore,
    bool saturatorActive,
    bool compressorActive,
    std::vector<RuntimeControlUpdate>& updates) const
{
    const auto& chain = stateStore.GetChain();
    const auto resolveDetector = [instanceId, &updates](
                                     dsp::DeviceId deviceId,
                                     bool parentActive,
                                     bool listen,
                                     const auto& filters)
    {
        bool hasSolo = false;
        for (const auto& filter : filters)
        {
            hasSolo = hasSolo || filter.solo.value;
        }
        for (std::size_t filterIndex = 0; filterIndex < filters.size(); ++filterIndex)
        {
            const auto& filter = filters[filterIndex];
            AppendActiveUpdate(
                DetectorFilterPath(instanceId, deviceId, filterIndex),
                (parentActive || listen) &&
                    !filter.bypass.value &&
                    (!hasSolo || filter.solo.value),
                updates);
        }
    };

    resolveDetector(
        dsp::DeviceId::Saturator,
        saturatorActive,
        chain.saturator.detector.listen.value,
        chain.saturator.detector.filters);
    resolveDetector(
        dsp::DeviceId::Compressor,
        compressorActive,
        chain.compressor.detector.listen.value,
        chain.compressor.detector.filters);
}

void ProcessingStateResolver::ResolveMonitoring(
    InstanceId instanceId,
    const StateStore& stateStore,
    std::vector<RuntimeControlUpdate>& updates) const
{
    const auto& chain = stateStore.GetChain();
    AppendMonitoringUpdate(
        DevicePath(instanceId, dsp::DeviceId::Saturator)
            .WithNode(dsp::RouteNodeId::Detector),
        chain.saturator.detector.listen.value,
        updates);
    AppendMonitoringUpdate(
        DevicePath(instanceId, dsp::DeviceId::Compressor)
            .WithNode(dsp::RouteNodeId::Detector),
        chain.compressor.detector.listen.value,
        updates);
}

void ProcessingStateResolver::AppendActiveUpdate(
    const StatePath& path,
    bool active,
    std::vector<RuntimeControlUpdate>& updates)
{
    updates.push_back(RuntimeControlUpdate{
        path,
        RuntimeProperty::Active,
        active,
        0});
}

void ProcessingStateResolver::AppendMonitoringUpdate(
    const StatePath& path,
    bool enabled,
    std::vector<RuntimeControlUpdate>& updates)
{
    updates.push_back(RuntimeControlUpdate{
        path,
        RuntimeProperty::Listen,
        enabled,
        0});
}

} // namespace consolidator::core
