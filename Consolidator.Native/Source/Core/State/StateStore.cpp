#include "Core/State/StateStore.h"

#include <cstdint>
#include <type_traits>
#include <utility>

#include "Core/Settings/DspDeviceSettings.h"

namespace consolidator::core
{

namespace
{

template <typename T>
ApplyResult ApplyParameter(
    const StatePath& path,
    const StateValue& value,
    dsp::DspParameter<T>& parameter)
{
    if (!path.parameterId || *path.parameterId != parameter.id)
    {
        return ApplyResult::NotHandled;
    }

    if (!std::holds_alternative<T>(value))
    {
        return ApplyResult::Rejected;
    }

    return parameter.Apply(
               path,
               dsp::ParameterValue{std::get<T>(value)})
        ? ApplyResult::Applied
        : ApplyResult::Unchanged;
}

template <typename T>
void AppendParameter(
    const StatePath& query,
    StateResponseEntries& snapshot,
    StatePath path,
    const dsp::DspParameter<T>& parameter)
{
    path.field = StateField::DspParameter;
    path.instanceId = query.instanceId;
    if (!query.Matches(path))
    {
        return;
    }

    StateEntry entry{path, StateValue{parameter.value}};
    if constexpr (!std::is_same_v<T, bool>)
    {
        entry.physicalMinimum = dsp::ParameterValue{parameter.minimum};
        entry.physicalMaximum = dsp::ParameterValue{parameter.maximum};
        entry.minimum = entry.physicalMinimum;
        entry.maximum = entry.physicalMaximum;
    }
    (void)snapshot.TryAppend(std::move(entry));
}

void AppendFilterState(
    const StatePath& query,
    StateResponseEntries& snapshot,
    StatePath basePath,
    const dsp::FilterState& state)
{
    AppendParameter(query, snapshot, basePath.WithParameter(dsp::ParameterId::Frequency), state.frequencyHz);
    AppendParameter(query, snapshot, basePath.WithParameter(dsp::ParameterId::Q), state.q);
    AppendParameter(query, snapshot, basePath.WithParameter(dsp::ParameterId::Gain), state.gainDb);
    AppendParameter(query, snapshot, basePath.WithParameter(dsp::ParameterId::Bypass), state.bypass);
}

void AppendGainState(
    const StatePath& query,
    StateResponseEntries& snapshot,
    dsp::DeviceId deviceId,
    const dsp::GainState& state)
{
    AppendParameter(query, snapshot, StatePath{deviceId, dsp::ParameterId::Gain}, state.gainDb);
    AppendParameter(query, snapshot, StatePath{deviceId, dsp::ParameterId::Bypass}, state.bypass);
}

} // namespace

StateStore::StateStore(InstanceState& topology) noexcept
    : topology_(topology)
{
    const core::settings::DspSettings settings{};
    for (std::size_t bankIndex = 0; bankIndex < settings.banks.size(); ++bankIndex)
    {
        for (std::size_t filterIndex = 0; filterIndex < settings.banks[bankIndex].bands.size(); ++filterIndex)
        {
            const auto& settingsFilter = settings.banks[bankIndex].bands[filterIndex];
            auto& state = chain_.equalizerFilters[bankIndex][filterIndex];
            state.frequencyHz = static_cast<float>(settingsFilter.frequencyHz.defaultValue);
            state.q = static_cast<float>(settingsFilter.q.defaultValue);
            state.gainDb = static_cast<float>(settingsFilter.gainDb.defaultValue);
            state.bypass = settingsFilter.bypass.defaultValue;
        }
    }

    for (std::size_t filterIndex = 0; filterIndex < settings.saturator.detector.bands.size(); ++filterIndex)
    {
        const auto& settingsFilter = settings.saturator.detector.bands[filterIndex];
        auto& state = chain_.saturatorDetectorFilters[filterIndex];
        state.frequencyHz = static_cast<float>(settingsFilter.frequencyHz.defaultValue);
        state.q = static_cast<float>(settingsFilter.q.defaultValue);
        state.gainDb = static_cast<float>(settingsFilter.gainDb.defaultValue);
        state.bypass = settingsFilter.bypass.defaultValue;
    }

    for (std::size_t filterIndex = 0; filterIndex < settings.compressor.detector.bands.size(); ++filterIndex)
    {
        const auto& settingsFilter = settings.compressor.detector.bands[filterIndex];
        auto& state = chain_.compressorDetectorFilters[filterIndex];
        state.frequencyHz = static_cast<float>(settingsFilter.frequencyHz.defaultValue);
        state.q = static_cast<float>(settingsFilter.q.defaultValue);
        state.gainDb = static_cast<float>(settingsFilter.gainDb.defaultValue);
        state.bypass = settingsFilter.bypass.defaultValue;
    }
}

ApplyResult StateStore::WriteGainState(
    const StatePath& path,
    const StateValue& value,
    dsp::GainState& state) const
{
    auto result = ApplyParameter(path, value, state.gainDb);
    if (result == ApplyResult::NotHandled)
    {
        result = ApplyParameter(path, value, state.bypass);
    }
    return result;
}

ApplyResult StateStore::WriteFilterState(
    const StatePath& path,
    const StateValue& value,
    dsp::FilterState& state) const
{
    auto result = ApplyParameter(path, value, state.frequencyHz);
    if (result == ApplyResult::NotHandled)
    {
        result = ApplyParameter(path, value, state.q);
    }
    if (result == ApplyResult::NotHandled)
    {
        result = ApplyParameter(path, value, state.gainDb);
    }
    if (result == ApplyResult::NotHandled)
    {
        result = ApplyParameter(path, value, state.bypass);
    }
    return result;
}

ApplyResult StateStore::WriteSaturatorState(
    const StatePath& path,
    const StateValue& value)
{
    if (path.depth == 0)
    {
        auto result = ApplyParameter(path, value, chain_.saturator.drive);
        if (result == ApplyResult::NotHandled)
        {
            result = ApplyParameter(path, value, chain_.saturator.outputDb);
        }
        if (result == ApplyResult::NotHandled)
        {
            result = ApplyParameter(path, value, chain_.saturator.mix);
        }
        if (result == ApplyResult::NotHandled)
        {
            result = ApplyParameter(path, value, chain_.saturator.detectorAmount);
        }
        if (result == ApplyResult::NotHandled)
        {
            result = ApplyParameter(path, value, chain_.saturator.bypass);
        }
        return result;
    }

    if (path.nodes[0] != dsp::RouteNodeId::Detector || path.depth <= 1)
    {
        return ApplyResult::NotHandled;
    }

    const auto filterIndex = static_cast<std::size_t>(path.nodes[1]) -
        static_cast<std::size_t>(dsp::RouteNodeId::Filter1);
    if (filterIndex >= chain_.saturatorDetectorFilters.size())
    {
        return ApplyResult::NotHandled;
    }
    return WriteFilterState(
        path,
        value,
        chain_.saturatorDetectorFilters[filterIndex]);
}

ApplyResult StateStore::WriteCompressorState(
    const StatePath& path,
    const StateValue& value)
{
    if (path.depth == 0)
    {
        auto result = ApplyParameter(path, value, chain_.compressor.thresholdDb);
        if (result == ApplyResult::NotHandled)
        {
            result = ApplyParameter(path, value, chain_.compressor.ratio);
        }
        if (result == ApplyResult::NotHandled)
        {
            result = ApplyParameter(path, value, chain_.compressor.attackMs);
        }
        if (result == ApplyResult::NotHandled)
        {
            result = ApplyParameter(path, value, chain_.compressor.releaseMs);
        }
        if (result == ApplyResult::NotHandled)
        {
            result = ApplyParameter(path, value, chain_.compressor.outputDb);
        }
        if (result == ApplyResult::NotHandled)
        {
            result = ApplyParameter(path, value, chain_.compressor.mix);
        }
        if (result == ApplyResult::NotHandled)
        {
            result = ApplyParameter(path, value, chain_.compressor.bypass);
        }
        return result;
    }

    if (path.nodes[0] != dsp::RouteNodeId::Detector || path.depth <= 1)
    {
        return ApplyResult::NotHandled;
    }

    const auto filterIndex = static_cast<std::size_t>(path.nodes[1]) -
        static_cast<std::size_t>(dsp::RouteNodeId::Filter1);
    if (filterIndex >= chain_.compressorDetectorFilters.size())
    {
        return ApplyResult::NotHandled;
    }
    return WriteFilterState(
        path,
        value,
        chain_.compressorDetectorFilters[filterIndex]);
}

ApplyResult StateStore::WriteEqualizerState(
    const StatePath& path,
    const StateValue& value)
{
    if (path.depth == 0)
    {
        return ApplyResult::NotHandled;
    }

    const auto bankIndex = static_cast<std::size_t>(path.nodes[0]) -
        static_cast<std::size_t>(dsp::RouteNodeId::Bank0);
    if (bankIndex >= chain_.equalizers.size())
    {
        return ApplyResult::NotHandled;
    }

    if (path.depth == 1)
    {
        return ApplyParameter(path, value, chain_.equalizers[bankIndex].bypass);
    }

    const auto filterIndex = static_cast<std::size_t>(path.nodes[1]) -
        static_cast<std::size_t>(dsp::RouteNodeId::Filter1);
    if (filterIndex >= chain_.equalizerFilters[bankIndex].size())
    {
        return ApplyResult::NotHandled;
    }
    return WriteFilterState(
        path,
        value,
        chain_.equalizerFilters[bankIndex][filterIndex]);
}

void StateStore::ReadState(
    const StatePath& path,
    StateResponseEntries& snapshot) const
{
    topology_.ReadState(path, snapshot);
    AppendGainState(path, snapshot, dsp::DeviceId::MainInputGain, chain_.inputGain);
    AppendGainState(path, snapshot, dsp::DeviceId::MainOutputGain, chain_.outputGain);

    AppendParameter(path, snapshot, StatePath{dsp::DeviceId::Saturator, dsp::ParameterId::Drive}, chain_.saturator.drive);
    AppendParameter(path, snapshot, StatePath{dsp::DeviceId::Saturator, dsp::ParameterId::Gain}, chain_.saturator.outputDb);
    AppendParameter(path, snapshot, StatePath{dsp::DeviceId::Saturator, dsp::ParameterId::Mix}, chain_.saturator.mix);
    AppendParameter(path, snapshot, StatePath{dsp::DeviceId::Saturator, dsp::ParameterId::Type}, chain_.saturator.detectorAmount);
    AppendParameter(path, snapshot, StatePath{dsp::DeviceId::Saturator, dsp::ParameterId::Bypass}, chain_.saturator.bypass);

    AppendParameter(path, snapshot, StatePath{dsp::DeviceId::Compressor, dsp::ParameterId::Threshold}, chain_.compressor.thresholdDb);
    AppendParameter(path, snapshot, StatePath{dsp::DeviceId::Compressor, dsp::ParameterId::Ratio}, chain_.compressor.ratio);
    AppendParameter(path, snapshot, StatePath{dsp::DeviceId::Compressor, dsp::ParameterId::Attack}, chain_.compressor.attackMs);
    AppendParameter(path, snapshot, StatePath{dsp::DeviceId::Compressor, dsp::ParameterId::Release}, chain_.compressor.releaseMs);
    AppendParameter(path, snapshot, StatePath{dsp::DeviceId::Compressor, dsp::ParameterId::Gain}, chain_.compressor.outputDb);
    AppendParameter(path, snapshot, StatePath{dsp::DeviceId::Compressor, dsp::ParameterId::Mix}, chain_.compressor.mix);
    AppendParameter(path, snapshot, StatePath{dsp::DeviceId::Compressor, dsp::ParameterId::Bypass}, chain_.compressor.bypass);

    for (std::size_t bankIndex = 0; bankIndex < chain_.equalizers.size(); ++bankIndex)
    {
        const auto bankNode = static_cast<dsp::RouteNodeId>(
            static_cast<std::uint8_t>(dsp::RouteNodeId::Bank0) + bankIndex);
        AppendParameter(
            path,
            snapshot,
            StatePath{dsp::DeviceId::Equalizer, dsp::ParameterId::Bypass, bankNode},
            chain_.equalizers[bankIndex].bypass);

        for (std::size_t filterIndex = 0; filterIndex < chain_.equalizerFilters[bankIndex].size(); ++filterIndex)
        {
            const auto filterNode = static_cast<dsp::RouteNodeId>(
                static_cast<std::uint8_t>(dsp::RouteNodeId::Filter1) + filterIndex);
            AppendFilterState(
                path,
                snapshot,
                StatePath{dsp::DeviceId::Equalizer, dsp::ParameterId::Gain, bankNode, filterNode},
                chain_.equalizerFilters[bankIndex][filterIndex]);
        }
    }

    for (std::size_t filterIndex = 0;
         filterIndex < chain_.saturatorDetectorFilters.size();
         ++filterIndex)
    {
        const auto filterNode = static_cast<dsp::RouteNodeId>(
            static_cast<std::uint8_t>(dsp::RouteNodeId::Filter1) + filterIndex);
        AppendFilterState(
            path,
            snapshot,
            StatePath{dsp::DeviceId::Saturator, dsp::ParameterId::Gain, dsp::RouteNodeId::Detector, filterNode},
            chain_.saturatorDetectorFilters[filterIndex]);
    }

    for (std::size_t filterIndex = 0;
         filterIndex < chain_.compressorDetectorFilters.size();
         ++filterIndex)
    {
        const auto filterNode = static_cast<dsp::RouteNodeId>(
            static_cast<std::uint8_t>(dsp::RouteNodeId::Filter1) + filterIndex);
        AppendFilterState(
            path,
            snapshot,
            StatePath{dsp::DeviceId::Compressor, dsp::ParameterId::Gain, dsp::RouteNodeId::Detector, filterNode},
            chain_.compressorDetectorFilters[filterIndex]);
    }
}

StateWriteStatus StateStore::WriteState(
    const StateEntry& entry,
    StateResponseEntries& applied)
{
    const auto topologyStatus = topology_.WriteState(entry, applied);
    if (topologyStatus != StateWriteStatus::NotHandled)
    {
        return topologyStatus;
    }

    if (entry.path.field != StateField::DspParameter ||
        !entry.path.deviceId ||
        !entry.path.parameterId)
    {
        return StateWriteStatus::NotHandled;
    }

    ApplyResult result = ApplyResult::NotHandled;
    const auto deviceId = *entry.path.deviceId;
    if (deviceId == dsp::DeviceId::MainInputGain)
    {
        result = WriteGainState(entry.path, entry.value, chain_.inputGain);
    }
    else if (deviceId == dsp::DeviceId::MainOutputGain)
    {
        result = WriteGainState(entry.path, entry.value, chain_.outputGain);
    }
    else if (deviceId == dsp::DeviceId::Saturator)
    {
        result = WriteSaturatorState(entry.path, entry.value);
    }
    else if (deviceId == dsp::DeviceId::Compressor)
    {
        result = WriteCompressorState(entry.path, entry.value);
    }
    else if (deviceId == dsp::DeviceId::Equalizer)
    {
        result = WriteEqualizerState(entry.path, entry.value);
    }

    if (result == ApplyResult::NotHandled)
    {
        return StateWriteStatus::NotHandled;
    }
    if (result == ApplyResult::Rejected)
    {
        StateEntry rejected{entry.path, entry.value};
        rejected.status = StateWriteStatus::Rejected;
        (void)applied.TryAppend(std::move(rejected));
        return StateWriteStatus::Rejected;
    }

    const auto previousSize = applied.size;
    ReadState(entry.path, applied);
    for (std::size_t index = previousSize; index < applied.size; ++index)
    {
        applied.entries[index].status = result == ApplyResult::Applied
            ? StateWriteStatus::Applied
            : StateWriteStatus::Unchanged;
    }
    return result == ApplyResult::Applied
        ? StateWriteStatus::Applied
        : StateWriteStatus::Unchanged;
}

bool StateStore::CanWrite(const StateEntry& entry) const
{
    if (entry.path.field != StateField::DspParameter)
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
