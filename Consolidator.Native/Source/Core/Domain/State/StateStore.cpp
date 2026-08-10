#include "Core/Domain/State/StateStore.h"

#include <cstdint>
#include <utility>

#include "Core/Settings/DspDeviceSettings.h"

namespace consolidator::core
{

StateStore::StateStore(InstanceState& topology) noexcept
    : topology_(topology)
{
    const core::settings::DspSettings settings{};
    for (std::size_t bankIndex = 0; bankIndex < settings.banks.size(); ++bankIndex)
    {
        for (std::size_t filterIndex = 0;
             filterIndex < settings.banks[bankIndex].bands.size();
             ++filterIndex)
        {
            const auto& settingsFilter = settings.banks[bankIndex].bands[filterIndex];
            auto& state = chain_.equalizerFilters[bankIndex][filterIndex];
            state.frequencyHz = static_cast<float>(settingsFilter.frequencyHz.defaultValue);
            state.q = static_cast<float>(settingsFilter.q.defaultValue);
            state.gainDb = static_cast<float>(settingsFilter.gainDb.defaultValue);
            state.bypass = settingsFilter.bypass.defaultValue;
        }
    }

    for (std::size_t filterIndex = 0;
         filterIndex < settings.saturator.detector.bands.size();
         ++filterIndex)
    {
        const auto& settingsFilter = settings.saturator.detector.bands[filterIndex];
        auto& state = chain_.saturatorDetectorFilters[filterIndex];
        state.frequencyHz = static_cast<float>(settingsFilter.frequencyHz.defaultValue);
        state.q = static_cast<float>(settingsFilter.q.defaultValue);
        state.gainDb = static_cast<float>(settingsFilter.gainDb.defaultValue);
        state.bypass = settingsFilter.bypass.defaultValue;
    }

    for (std::size_t filterIndex = 0;
         filterIndex < settings.compressor.detector.bands.size();
         ++filterIndex)
    {
        const auto& settingsFilter = settings.compressor.detector.bands[filterIndex];
        auto& state = chain_.compressorDetectorFilters[filterIndex];
        state.frequencyHz = static_cast<float>(settingsFilter.frequencyHz.defaultValue);
        state.q = static_cast<float>(settingsFilter.q.defaultValue);
        state.gainDb = static_cast<float>(settingsFilter.gainDb.defaultValue);
        state.bypass = settingsFilter.bypass.defaultValue;
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
