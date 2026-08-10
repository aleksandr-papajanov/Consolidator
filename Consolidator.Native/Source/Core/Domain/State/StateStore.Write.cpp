#include "Core/Domain/State/StateStore.h"

#include <algorithm>
#include <type_traits>
#include <utility>

namespace consolidator::core
{

enum class ApplyResult
{
    NotHandled,
    Rejected,
    Unchanged,
    Applied
};

namespace detail
{

template <typename T>
ApplyResult ApplyParameter(
    const StatePath& path,
    const StateValue& value,
    dsp::ParameterState<T>& parameter)
{
    if (!path.parameterId || *path.parameterId != parameter.id)
    {
        return ApplyResult::NotHandled;
    }

    const auto* updatedValue = std::get_if<T>(&value);
    if (updatedValue == nullptr)
    {
        return ApplyResult::Rejected;
    }

    if constexpr (std::is_same_v<T, bool>)
    {
        if (parameter.value == *updatedValue)
        {
            return ApplyResult::Unchanged;
        }
        parameter.value = *updatedValue;
    }
    else
    {
        const auto clampedValue = std::clamp(
            *updatedValue,
            parameter.minimum,
            parameter.maximum);
        if (parameter.value == clampedValue)
        {
            return ApplyResult::Unchanged;
        }
        parameter.value = clampedValue;
    }

    return ApplyResult::Applied;
}

template <typename... Parameters>
ApplyResult ApplyOne(
    const StatePath& path,
    const StateValue& value,
    Parameters&... parameters)
{
    ApplyResult result = ApplyResult::NotHandled;
    ((result == ApplyResult::NotHandled
          ? result = ApplyParameter(path, value, parameters)
          : result),
     ...);
    return result;
}

} // namespace detail

StateWriteStatus StateStore::WriteState(
    const StateEntry& entry,
    StateResponseEntries& applied)
{
    if (const auto result = WriteInstanceState(entry, applied);
        result != StateWriteStatus::NotHandled)
    {
        return result;
    }

    if (entry.path.field != StateField::DspParameter)
    {
        return StateWriteStatus::NotHandled;
    }

    return AppendWriteResult(entry, WriteDspState(entry), applied);
}

StateWriteStatus StateStore::WriteInstanceState(
    const StateEntry& entry,
    StateResponseEntries& applied)
{
    if (!entry.path.field)
    {
        return StateWriteStatus::NotHandled;
    }

    if (*entry.path.field == StateField::SelectedBank)
    {
        return WriteSelectedBank(entry, applied);
    }
    if (*entry.path.field == StateField::GroupId)
    {
        return WriteBankGroup(entry, applied);
    }
    return StateWriteStatus::NotHandled;
}

StateWriteStatus StateStore::WriteSelectedBank(
    const StateEntry& entry,
    StateResponseEntries& applied)
{
    const auto* value = std::get_if<dsp::BankId>(&entry.value);
    if (value == nullptr)
    {
        return StateWriteStatus::Rejected;
    }

    const auto status = instance_.selectedBankId == *value
        ? StateWriteStatus::Unchanged
        : StateWriteStatus::Applied;
    instance_.selectedBankId = *value;
    StateEntry result{entry.path, StateValue{*value}};
    result.status = status;
    (void)applied.TryAppend(std::move(result));
    return status;
}

StateWriteStatus StateStore::WriteBankGroup(
    const StateEntry& entry,
    StateResponseEntries& applied)
{
    const auto bankId = entry.path.TryGetBankId();
    if (!bankId ||
        (!std::holds_alternative<GroupId>(entry.value) &&
         !std::holds_alternative<std::monostate>(entry.value)))
    {
        return StateWriteStatus::Rejected;
    }

    auto& bank = instance_.banks[dsp::detail::ToIndex(*bankId)];
    const auto nextGroup = std::holds_alternative<GroupId>(entry.value)
        ? std::optional<GroupId>{std::get<GroupId>(entry.value)}
        : std::nullopt;
    const auto status = bank.groupId == nextGroup
        ? StateWriteStatus::Unchanged
        : StateWriteStatus::Applied;
    bank.groupId = nextGroup;
    StateEntry result{entry.path, entry.value};
    result.status = status;
    (void)applied.TryAppend(std::move(result));
    return status;
}

ApplyResult StateStore::WriteDspState(const StateEntry& entry)
{
    if (!entry.path.deviceId || !entry.path.parameterId)
    {
        return ApplyResult::NotHandled;
    }

    const auto deviceId = *entry.path.deviceId;
    if (deviceId == dsp::DeviceId::MainInputGain)
    {
        return WriteGainState(entry.path, entry.value, chain_.inputGain);
    }
    if (deviceId == dsp::DeviceId::MainOutputGain)
    {
        return WriteGainState(entry.path, entry.value, chain_.outputGain);
    }
    if (deviceId == dsp::DeviceId::Saturator)
    {
        return WriteSaturatorState(entry.path, entry.value);
    }
    if (deviceId == dsp::DeviceId::Compressor)
    {
        return WriteCompressorState(entry.path, entry.value);
    }
    if (deviceId == dsp::DeviceId::Equalizer)
    {
        return WriteEqualizerState(entry.path, entry.value);
    }
    return ApplyResult::NotHandled;
}

StateWriteStatus StateStore::AppendWriteResult(
    const StateEntry& entry,
    ApplyResult result,
    StateResponseEntries& applied)
{
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

ApplyResult StateStore::WriteGainState(
    const StatePath& path,
    const StateValue& value,
    dsp::GainState& state)
{
    return detail::ApplyOne(path, value, state.gainDb, state.bypass);
}

ApplyResult StateStore::WriteFilterState(
    const StatePath& path,
    const StateValue& value,
    dsp::FilterState& state)
{
    return detail::ApplyOne(
        path,
        value,
        state.frequencyHz,
        state.q,
        state.gainDb,
        state.bypass);
}

ApplyResult StateStore::WriteSaturatorState(
    const StatePath& path,
    const StateValue& value)
{
    if (path.depth == 0)
    {
        return detail::ApplyOne(
            path,
            value,
            chain_.saturator.drive,
            chain_.saturator.outputDb,
            chain_.saturator.mix,
            chain_.saturator.detectorAmount,
            chain_.saturator.bypass);
    }

    if (path.nodes[0] != dsp::RouteNodeId::Detector || path.depth <= 1)
    {
        return ApplyResult::NotHandled;
    }

    const auto filterIndex = static_cast<std::size_t>(path.nodes[1]) -
        static_cast<std::size_t>(dsp::RouteNodeId::Filter1);
    if (filterIndex >= chain_.saturator.detector.filters.size())
    {
        return ApplyResult::NotHandled;
    }
    return WriteFilterState(
        path,
        value,
        chain_.saturator.detector.filters[filterIndex]);
}

ApplyResult StateStore::WriteCompressorState(
    const StatePath& path,
    const StateValue& value)
{
    if (path.depth == 0)
    {
        return detail::ApplyOne(
            path,
            value,
            chain_.compressor.thresholdDb,
            chain_.compressor.ratio,
            chain_.compressor.attackMs,
            chain_.compressor.releaseMs,
            chain_.compressor.outputDb,
            chain_.compressor.mix,
            chain_.compressor.bypass);
    }

    if (path.nodes[0] != dsp::RouteNodeId::Detector || path.depth <= 1)
    {
        return ApplyResult::NotHandled;
    }

    const auto filterIndex = static_cast<std::size_t>(path.nodes[1]) -
        static_cast<std::size_t>(dsp::RouteNodeId::Filter1);
    if (filterIndex >= chain_.compressor.detector.filters.size())
    {
        return ApplyResult::NotHandled;
    }
    return WriteFilterState(
        path,
        value,
        chain_.compressor.detector.filters[filterIndex]);
}

ApplyResult StateStore::WriteEqualizerState(
    const StatePath& path,
    const StateValue& value)
{
    if (path.depth == 0)
    {
        return ApplyResult::NotHandled;
    }

    const auto bankId = path.TryGetBankId();
    if (!bankId)
    {
        return ApplyResult::NotHandled;
    }

    const auto bankIndex = dsp::detail::ToIndex(*bankId);

    if (path.depth == 1)
    {
        return detail::ApplyOne(
            path,
            value,
            chain_.equalizers[bankIndex].state.bypass);
    }

    const auto filterIndex = static_cast<std::size_t>(path.nodes[1]) -
        static_cast<std::size_t>(dsp::RouteNodeId::Filter1);
    if (filterIndex >= chain_.equalizers[bankIndex].filters.size())
    {
        return ApplyResult::NotHandled;
    }
    return WriteFilterState(
        path,
        value,
        chain_.equalizers[bankIndex].filters[filterIndex]);
}

} // namespace consolidator::core
