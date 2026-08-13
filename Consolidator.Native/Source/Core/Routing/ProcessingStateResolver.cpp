#include "Core/Routing/ProcessingStateResolver.h"

#include <cstdint>

namespace consolidator::core
{

namespace
{

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

StatePath FilterPath(InstanceId instanceId, dsp::BankId bankId,
                     std::size_t filterIndex)
{
    return BankPath(instanceId, bankId).WithNode(
        static_cast<dsp::RouteNodeId>(
            static_cast<std::uint8_t>(dsp::RouteNodeId::Filter1) +
            static_cast<std::uint8_t>(filterIndex)));
}

StatePath DetectorFilterPath(InstanceId instanceId, dsp::DeviceId deviceId,
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
    const auto& chain = stateStore.GetChain();
    const auto state = ResolveProcessingState(chain);

    AppendActiveUpdate(DevicePath(instanceId, dsp::DeviceId::MainInputGain),
                       !chain.inputGain.bypass.value, resolution.controls);
    AppendActiveUpdate(DevicePath(instanceId, dsp::DeviceId::Saturator),
                       state.saturatorActive, resolution.controls);
    AppendActiveUpdate(DevicePath(instanceId, dsp::DeviceId::Compressor),
                       state.compressorActive, resolution.controls);
    AppendActiveUpdate(DevicePath(instanceId, dsp::DeviceId::Equalizer),
                       state.equalizerActive, resolution.controls);
    AppendActiveUpdate(DevicePath(instanceId, dsp::DeviceId::MainOutputGain),
                       !chain.outputGain.bypass.value, resolution.controls);

    for (std::size_t bankIndex = 0;
         bankIndex < state.equalizerBanksActive.size(); ++bankIndex)
    {
        const auto bankId = static_cast<dsp::BankId>(bankIndex);
        AppendActiveUpdate(BankPath(instanceId, bankId),
                           state.equalizerBanksActive[bankIndex],
                           resolution.controls);
        for (std::size_t filterIndex = 0; filterIndex < 7; ++filterIndex)
        {
            AppendActiveUpdate(
                FilterPath(instanceId, bankId, filterIndex),
                state.equalizerFiltersActive[bankIndex][filterIndex],
                resolution.controls);
        }
    }

    for (std::size_t filterIndex = 0; filterIndex < 2; ++filterIndex)
    {
        AppendActiveUpdate(
            DetectorFilterPath(instanceId, dsp::DeviceId::Saturator,
                               filterIndex),
            state.saturatorDetectorFiltersActive[filterIndex],
            resolution.controls);
        AppendActiveUpdate(
            DetectorFilterPath(instanceId, dsp::DeviceId::Compressor,
                               filterIndex),
            state.compressorDetectorFiltersActive[filterIndex],
            resolution.controls);
    }

    AppendMonitoringUpdate(
        DevicePath(instanceId, dsp::DeviceId::Saturator)
            .WithNode(dsp::RouteNodeId::Detector),
        chain.saturator.detector.listen.value, resolution.controls);
    AppendMonitoringUpdate(
        DevicePath(instanceId, dsp::DeviceId::Compressor)
            .WithNode(dsp::RouteNodeId::Detector),
        chain.compressor.detector.listen.value, resolution.controls);
}

void ProcessingStateResolver::AppendActiveUpdate(
    const StatePath& path,
    bool active,
    std::vector<RuntimeControlUpdate>& updates)
{
    updates.push_back(RuntimeControlUpdate{path, RuntimeProperty::Active,
                                            active, 0});
}

void ProcessingStateResolver::AppendMonitoringUpdate(
    const StatePath& path,
    bool enabled,
    std::vector<RuntimeControlUpdate>& updates)
{
    updates.push_back(RuntimeControlUpdate{path, RuntimeProperty::Listen,
                                            enabled, 0});
}

} // namespace consolidator::core
