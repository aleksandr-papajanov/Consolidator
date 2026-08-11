#include "Core/Domain/State/StateStore.h"

#include <cstdint>
#include <type_traits>
#include <utility>

namespace consolidator::core
{

namespace detail
{

template <typename T>
void AppendParameter(
    const StatePath& query,
    StateResponseEntries& snapshot,
    StatePath path,
    const dsp::ParameterState<T>& parameter)
{
    path.field = StateField::DspParameter;
    path.instanceId = query.instanceId;
    path.parameterId = parameter.id;
    if (!query.Matches(path))
    {
        return;
    }

    StateEntry entry{path, StateValue{parameter.value}};
    if constexpr (!std::is_same_v<T, bool>)
    {
        entry.physicalMinimum = dsp::ParameterVariant{parameter.minimum};
        entry.physicalMaximum = dsp::ParameterVariant{parameter.maximum};
        entry.minimum = entry.physicalMinimum;
        entry.maximum = entry.physicalMaximum;
    }
    (void)snapshot.TryAppend(std::move(entry));
}

template <typename... Parameters>
void AppendParameters(
    const StatePath& query,
    StateResponseEntries& snapshot,
    StatePath path,
    const Parameters&... parameters)
{
    (AppendParameter(query, snapshot, path, parameters), ...);
}

void AppendMarker(
    const StatePath& query,
    StateResponseEntries& snapshot,
    StatePath path,
    dsp::ParameterId parameterId,
    const dsp::StateMarker<bool>& marker)
{
    path.field = StateField::DspParameter;
    path.instanceId = query.instanceId;
    path.parameterId = parameterId;
    if (query.Matches(path))
    {
        (void)snapshot.TryAppend(StateEntry{path, StateValue{marker.value}});
    }
}

} // namespace detail

namespace
{

void AppendInstanceState(
    const StatePath& query,
    StateResponseEntries& snapshot,
    const InstanceState& state)
{
    const auto instancePath = StatePath::Instance(state.instanceId);
    if (query.Matches(instancePath))
    {
        (void)snapshot.TryAppend(StateEntry{
            instancePath,
            StateValue{state.instanceId}});
    }

    auto selectedBankPath = instancePath;
    selectedBankPath.field = StateField::SelectedBank;
    if (query.Matches(selectedBankPath))
    {
        (void)snapshot.TryAppend(StateEntry{
            selectedBankPath,
            StateValue{state.selectedBankId}});
    }

    for (const auto& bank : state.banks)
    {
        auto bankPath = instancePath;
        bankPath.field = StateField::BankId;
        bankPath.nodes[0] = static_cast<dsp::RouteNodeId>(
            static_cast<std::uint8_t>(dsp::RouteNodeId::Bank0) +
            dsp::detail::ToIndex(bank.id));
        bankPath.depth = 1;
        if (query.Matches(bankPath))
        {
            (void)snapshot.TryAppend(StateEntry{
                bankPath,
                StateValue{bank.id}});
        }

        auto groupPath = bankPath;
        groupPath.field = StateField::GroupId;
        if (query.Matches(groupPath))
        {
            const StateValue value = bank.groupId
                ? StateValue{*bank.groupId}
                : StateValue{std::monostate{}};
            (void)snapshot.TryAppend(StateEntry{groupPath, value});
        }
    }

    const auto mutePath = StatePath::InstanceMute(state.instanceId);
    if (query.Matches(mutePath))
    {
        (void)snapshot.TryAppend(StateEntry{
            mutePath,
            StateValue{state.audibility.mute.value}});
    }
    const auto soloPath = StatePath::InstanceSolo(state.instanceId);
    if (query.Matches(soloPath))
    {
        (void)snapshot.TryAppend(StateEntry{
            soloPath,
            StateValue{state.audibility.solo.value}});
    }
}

void AppendFilterState(
    const StatePath& query,
    StateResponseEntries& snapshot,
    StatePath basePath,
    const dsp::FilterState& state)
{
    detail::AppendParameters(query, snapshot, basePath,
                             state.frequencyHz, state.q, state.gainDb);
    detail::AppendMarker(query, snapshot, basePath,
                         dsp::ParameterId::Bypass, state.bypass);
    detail::AppendMarker(query, snapshot, basePath,
                         dsp::ParameterId::Solo, state.solo);
}

void AppendGainState(
    const StatePath& query,
    StateResponseEntries& snapshot,
    dsp::DeviceId deviceId,
    const dsp::GainState& state)
{
    detail::AppendParameters(query, snapshot, StatePath::Device(deviceId), state.gainDb);
    detail::AppendMarker(query, snapshot, StatePath::Device(deviceId),
                         dsp::ParameterId::Bypass, state.bypass);
}

} // namespace

void StateStore::ReadState(
    const StatePath& path,
    StateResponseEntries& snapshot) const
{
    AppendInstanceState(path, snapshot, instance_);
    AppendGainState(path, snapshot, dsp::DeviceId::MainInputGain, chain_.inputGain);
    AppendGainState(path, snapshot, dsp::DeviceId::MainOutputGain, chain_.outputGain);

    detail::AppendMarker(path, snapshot,
        StatePath::Device(dsp::DeviceId::Equalizer),
        dsp::ParameterId::Bypass, chain_.equalizer.bypass);
    detail::AppendMarker(path, snapshot,
        StatePath::Device(dsp::DeviceId::Equalizer),
        dsp::ParameterId::Solo, chain_.equalizer.solo);

    for (std::size_t bankIndex = 0; bankIndex < chain_.equalizers.size(); ++bankIndex)
    {
        const auto bankNode = static_cast<dsp::RouteNodeId>(
            static_cast<std::uint8_t>(dsp::RouteNodeId::Bank0) + bankIndex);
        const auto bankPath = StatePath::Device(dsp::DeviceId::Equalizer)
            .WithNode(bankNode);
        detail::AppendMarker(path, snapshot, bankPath,
            dsp::ParameterId::Bypass, chain_.equalizers[bankIndex].bypass);
        detail::AppendMarker(path, snapshot, bankPath,
            dsp::ParameterId::Solo, chain_.equalizers[bankIndex].solo);

        for (std::size_t filterIndex = 0;
             filterIndex < chain_.equalizers[bankIndex].filters.size();
             ++filterIndex)
        {
            const auto filterNode = static_cast<dsp::RouteNodeId>(
                static_cast<std::uint8_t>(dsp::RouteNodeId::Filter1) + filterIndex);
            AppendFilterState(
                path,
                snapshot,
                StatePath::Device(dsp::DeviceId::Equalizer)
                    .WithNode(bankNode)
                    .WithNode(filterNode),
                chain_.equalizers[bankIndex].filters[filterIndex]);
        }
    }

    detail::AppendParameters(path, snapshot,
        StatePath::Device(dsp::DeviceId::Saturator),
        chain_.saturator.drive, chain_.saturator.outputDb,
        chain_.saturator.mix, chain_.saturator.detectorAmount);
    detail::AppendMarker(path, snapshot,
        StatePath::Device(dsp::DeviceId::Saturator),
        dsp::ParameterId::Bypass, chain_.saturator.bypass);
    detail::AppendMarker(path, snapshot,
        StatePath::Device(dsp::DeviceId::Saturator),
        dsp::ParameterId::Solo, chain_.saturator.solo);
    detail::AppendParameters(
        path,
        snapshot,
        StatePath::Device(dsp::DeviceId::Compressor),
        chain_.compressor.thresholdDb,
        chain_.compressor.ratio,
        chain_.compressor.attackMs,
        chain_.compressor.releaseMs,
        chain_.compressor.outputDb,
        chain_.compressor.mix);
    detail::AppendMarker(path, snapshot,
        StatePath::Device(dsp::DeviceId::Compressor),
        dsp::ParameterId::Bypass, chain_.compressor.bypass);
    detail::AppendMarker(path, snapshot,
        StatePath::Device(dsp::DeviceId::Compressor),
        dsp::ParameterId::Solo, chain_.compressor.solo);

    for (std::size_t filterIndex = 0;
         filterIndex < chain_.saturator.detector.filters.size();
         ++filterIndex)
    {
        const auto filterNode = static_cast<dsp::RouteNodeId>(
            static_cast<std::uint8_t>(dsp::RouteNodeId::Filter1) + filterIndex);
        AppendFilterState(
            path,
            snapshot,
            StatePath::Device(dsp::DeviceId::Saturator)
                .WithNode(dsp::RouteNodeId::Detector)
                .WithNode(filterNode),
            chain_.saturator.detector.filters[filterIndex]);
    }

    detail::AppendMarker(path, snapshot,
        StatePath::Device(dsp::DeviceId::Saturator)
            .WithNode(dsp::RouteNodeId::Detector),
        dsp::ParameterId::Listen, chain_.saturator.detector.listen);

    for (std::size_t filterIndex = 0;
         filterIndex < chain_.compressor.detector.filters.size();
         ++filterIndex)
    {
        const auto filterNode = static_cast<dsp::RouteNodeId>(
            static_cast<std::uint8_t>(dsp::RouteNodeId::Filter1) + filterIndex);
        AppendFilterState(
            path,
            snapshot,
            StatePath::Device(dsp::DeviceId::Compressor)
                .WithNode(dsp::RouteNodeId::Detector)
                .WithNode(filterNode),
            chain_.compressor.detector.filters[filterIndex]);
    }

    detail::AppendMarker(path, snapshot,
        StatePath::Device(dsp::DeviceId::Compressor)
            .WithNode(dsp::RouteNodeId::Detector),
        dsp::ParameterId::Listen, chain_.compressor.detector.listen);
}

bool StateStore::CanWrite(const StateEntry& entry) const
{
    if (!entry.path.field)
    {
        return false;
    }

    StateResponseEntries current;
    ReadState(entry.path, current);
    if (current.size == 0)
    {
        return false;
    }

    const auto& currentValue = current.entries[0].value;
    if (std::holds_alternative<float>(currentValue))
    {
        return std::holds_alternative<float>(entry.value);
    }
    if (std::holds_alternative<bool>(currentValue))
    {
        return std::holds_alternative<bool>(entry.value);
    }
    return false;
}

} // namespace consolidator::core
