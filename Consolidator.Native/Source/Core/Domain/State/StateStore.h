#pragma once

#include "Core/Domain/State/ChainState.h"
#include "Core/Domain/State/StateEntry.h"
#include "Core/Domain/State/StateMarker.h"

namespace consolidator::core
{

enum class ApplyResult;

// Coordinator-owned source of truth for instance topology and DSP parameters.
class StateStore final
{
public:
    StateStore() noexcept;

    [[nodiscard]] InstanceState& GetInstance() noexcept
    {
        return instance_;
    }

    [[nodiscard]] const InstanceState& GetInstance() const noexcept
    {
        return instance_;
    }

    [[nodiscard]] const ChainState& GetChain() const noexcept
    {
        return chain_;
    }

    void SetInstanceId(InstanceId instanceId) noexcept
    {
        instance_.instanceId = instanceId;
    }

    [[nodiscard]] InstanceId GetInstanceId() const noexcept
    {
        return instance_.instanceId;
    }

    // Reads all entries matching the path prefix into a bounded snapshot.
    void ReadState(
        const StatePath& path,
        StateResponseEntries& snapshot) const;

    // Validates and applies one state entry, reporting the resulting status.
    StateWriteStatus WriteState(
        const StateEntry& entry,
        StateResponseEntries& applied);

    [[nodiscard]] bool CanWrite(const StateEntry& entry) const;

private:
    [[nodiscard]] StateWriteStatus WriteInstanceState(
        const StateEntry& entry,
        StateResponseEntries& applied);

    [[nodiscard]] StateWriteStatus WriteSelectedBank(
        const StateEntry& entry,
        StateResponseEntries& applied);

    [[nodiscard]] StateWriteStatus WriteBankGroup(
        const StateEntry& entry,
        StateResponseEntries& applied);

    [[nodiscard]] ApplyResult WriteDspState(const StateEntry& entry);

    [[nodiscard]] StateWriteStatus AppendWriteResult(
        const StateEntry& entry,
        ApplyResult result,
        StateResponseEntries& applied);

    [[nodiscard]] ApplyResult WriteGainState(
        const StatePath& path,
        const StateValue& value,
        dsp::GainState& state);

    [[nodiscard]] ApplyResult WriteFilterState(
        const StatePath& path,
        const StateValue& value,
        dsp::FilterState& state);

    [[nodiscard]] ApplyResult WriteSaturatorState(
        const StatePath& path,
        const StateValue& value);

    [[nodiscard]] ApplyResult WriteCompressorState(
        const StatePath& path,
        const StateValue& value);

    [[nodiscard]] ApplyResult WriteEqualizerState(
        const StatePath& path,
        const StateValue& value);

    InstanceState instance_;
    ChainState chain_;
};

} // namespace consolidator::core
