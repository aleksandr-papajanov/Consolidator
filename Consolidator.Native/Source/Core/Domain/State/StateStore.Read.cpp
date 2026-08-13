#include "Core/Domain/State/StateStore.h"

#include <cstdint>
#include <type_traits>
#include <utility>

namespace consolidator::core
{

namespace detail
{

void AppendEntry(StateResponseEntries& snapshot, StateEntry entry)
{
    (void)snapshot.TryAppend(std::move(entry));
}

void AppendEntry(std::vector<StateEntry>& snapshot, StateEntry entry)
{
    snapshot.push_back(std::move(entry));
}

template <typename Sink, typename T>
void AppendParameter(
    const StatePath& query,
    Sink& snapshot,
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
    AppendEntry(snapshot, std::move(entry));
}

template <typename Sink>
void AppendMarker(
    const StatePath& query,
    Sink& snapshot,
    StatePath path,
    StateMarkerId markerId,
    const dsp::StateMarker<bool>& marker)
{
    path.field = StateField::DspMarker;
    path.instanceId = query.instanceId;
    path.markerId = markerId;
    if (query.Matches(path))
    {
        AppendEntry(snapshot, StateEntry{path, StateValue{marker.value}});
    }
}

template <typename Visitor>
void VisitDspParameters(
    const ChainState& chain,
    Visitor&& visitor)
{
    visitor(StatePath::Device(dsp::DeviceId::MainInputGain), chain.inputGain.gainDb);
    visitor(StatePath::Device(dsp::DeviceId::MainOutputGain), chain.outputGain.gainDb);

    for (std::size_t bankIndex = 0; bankIndex < chain.equalizers.size(); ++bankIndex)
    {
        const auto bankNode = static_cast<dsp::RouteNodeId>(
            static_cast<std::uint8_t>(dsp::RouteNodeId::Bank0) + bankIndex);
        for (std::size_t filterIndex = 0;
             filterIndex < chain.equalizers[bankIndex].filters.size();
             ++filterIndex)
        {
            const auto filterNode = static_cast<dsp::RouteNodeId>(
                static_cast<std::uint8_t>(dsp::RouteNodeId::Filter1) + filterIndex);
            const auto path = StatePath::Device(dsp::DeviceId::Equalizer)
                .WithNode(bankNode)
                .WithNode(filterNode);
            const auto& filter = chain.equalizers[bankIndex].filters[filterIndex];
            visitor(path, filter.frequencyHz);
            visitor(path, filter.q);
            visitor(path, filter.gainDb);
        }
    }

    const auto saturatorPath = StatePath::Device(dsp::DeviceId::Saturator);
    visitor(saturatorPath, chain.saturator.drive);
    visitor(saturatorPath, chain.saturator.outputDb);
    visitor(saturatorPath, chain.saturator.mix);
    visitor(saturatorPath, chain.saturator.detectorAmount);

    const auto compressorPath = StatePath::Device(dsp::DeviceId::Compressor);
    visitor(compressorPath, chain.compressor.thresholdDb);
    visitor(compressorPath, chain.compressor.ratio);
    visitor(compressorPath, chain.compressor.attackMs);
    visitor(compressorPath, chain.compressor.releaseMs);
    visitor(compressorPath, chain.compressor.outputDb);
    visitor(compressorPath, chain.compressor.mix);

    for (std::size_t filterIndex = 0;
         filterIndex < chain.saturator.detector.filters.size();
         ++filterIndex)
    {
        const auto filterNode = static_cast<dsp::RouteNodeId>(
            static_cast<std::uint8_t>(dsp::RouteNodeId::Filter1) + filterIndex);
        const auto path = saturatorPath
            .WithNode(dsp::RouteNodeId::Detector)
            .WithNode(filterNode);
        const auto& filter = chain.saturator.detector.filters[filterIndex];
        visitor(path, filter.frequencyHz);
        visitor(path, filter.q);
        visitor(path, filter.gainDb);
    }

    for (std::size_t filterIndex = 0;
         filterIndex < chain.compressor.detector.filters.size();
         ++filterIndex)
    {
        const auto filterNode = static_cast<dsp::RouteNodeId>(
            static_cast<std::uint8_t>(dsp::RouteNodeId::Filter1) + filterIndex);
        const auto path = compressorPath
            .WithNode(dsp::RouteNodeId::Detector)
            .WithNode(filterNode);
        const auto& filter = chain.compressor.detector.filters[filterIndex];
        visitor(path, filter.frequencyHz);
        visitor(path, filter.q);
        visitor(path, filter.gainDb);
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

    const auto labelPath = StatePath::Label(state.instanceId);
    if (query.Matches(labelPath))
    {
        (void)snapshot.TryAppend(StateEntry{
            labelPath,
            StateValue{state.label}});
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
    detail::AppendMarker(query, snapshot, basePath,
                         StateMarkerId::Bypass, state.bypass);
    detail::AppendMarker(query, snapshot, basePath,
                         StateMarkerId::Solo, state.solo);
}

void AppendGainState(
    const StatePath& query,
    StateResponseEntries& snapshot,
    dsp::DeviceId deviceId,
    const dsp::GainState& state)
{
    detail::AppendMarker(query, snapshot, StatePath::Device(deviceId),
                         StateMarkerId::Bypass, state.bypass);
}

} // namespace

void StateStore::ReadState(
    const StatePath& path,
    StateResponseEntries& snapshot) const
{
    AppendInstanceState(path, snapshot, instance_);
    AppendGainState(path, snapshot, dsp::DeviceId::MainInputGain, chain_.inputGain);
    AppendGainState(path, snapshot, dsp::DeviceId::MainOutputGain, chain_.outputGain);
    detail::VisitDspParameters(chain_, [&path, &snapshot](const auto parameterPath,
                                                          const auto& parameter)
    {
        detail::AppendParameter(path, snapshot, parameterPath, parameter);
    });

    detail::AppendMarker(path, snapshot,
        StatePath::Device(dsp::DeviceId::Equalizer),
        StateMarkerId::Bypass, chain_.equalizer.bypass);
    detail::AppendMarker(path, snapshot,
        StatePath::Device(dsp::DeviceId::Equalizer),
        StateMarkerId::Solo, chain_.equalizer.solo);

    for (std::size_t bankIndex = 0; bankIndex < chain_.equalizers.size(); ++bankIndex)
    {
        const auto bankNode = static_cast<dsp::RouteNodeId>(
            static_cast<std::uint8_t>(dsp::RouteNodeId::Bank0) + bankIndex);
        const auto bankPath = StatePath::Device(dsp::DeviceId::Equalizer)
            .WithNode(bankNode);
        detail::AppendMarker(path, snapshot, bankPath,
            StateMarkerId::Bypass, chain_.equalizers[bankIndex].bypass);
        detail::AppendMarker(path, snapshot, bankPath,
            StateMarkerId::Solo, chain_.equalizers[bankIndex].solo);

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

    detail::AppendMarker(path, snapshot,
        StatePath::Device(dsp::DeviceId::Saturator),
        StateMarkerId::Bypass, chain_.saturator.bypass);
    detail::AppendMarker(path, snapshot,
        StatePath::Device(dsp::DeviceId::Saturator),
        StateMarkerId::Solo, chain_.saturator.solo);
    detail::AppendMarker(path, snapshot,
        StatePath::Device(dsp::DeviceId::Compressor),
        StateMarkerId::Bypass, chain_.compressor.bypass);
    detail::AppendMarker(path, snapshot,
        StatePath::Device(dsp::DeviceId::Compressor),
        StateMarkerId::Solo, chain_.compressor.solo);

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
        StateMarkerId::Listen, chain_.saturator.detector.listen);

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
        StateMarkerId::Listen, chain_.compressor.detector.listen);
}

void StateStore::ReadRuntimeParameters(std::vector<StateEntry>& parameters) const
{
    parameters.clear();
    const auto query = StatePath::Instance(instance_.instanceId);
    detail::VisitDspParameters(chain_, [&query, &parameters](const auto parameterPath,
                                                              const auto& parameter)
    {
        detail::AppendParameter(query, parameters, parameterPath, parameter);
    });
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
