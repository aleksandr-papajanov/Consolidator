#pragma once

#include <array>
#include <cstddef>

#include "Core/State/CompressorState.h"
#include "Core/State/EqualizerState.h"
#include "Core/State/FilterState.h"
#include "Core/State/GainState.h"
#include "Core/State/InstanceState.h"
#include "Core/State/SaturatorState.h"

namespace consolidator::core
{

struct ChainState
{
    dsp::GainState inputGain;
    dsp::GainState outputGain;
    dsp::SaturatorState saturator;
    dsp::CompressorState compressor;
    std::array<dsp::EqualizerState, InstanceState::kBankCount> equalizers;
    std::array<std::array<dsp::FilterState, 7>, InstanceState::kBankCount> equalizerFilters;
    std::array<dsp::FilterState, 2> saturatorDetectorFilters;
    std::array<dsp::FilterState, 2> compressorDetectorFilters;
};

enum class ApplyResult
{
    NotHandled,
    Rejected,
    Unchanged,
    Applied
};

class StateStore final
{
public:
    explicit StateStore(InstanceState& topology) noexcept;

    [[nodiscard]] InstanceId GetInstanceId() const noexcept
    {
        return topology_.GetInstanceId();
    }

    [[nodiscard]] InstanceState& GetTopology() noexcept
    {
        return topology_;
    }

    [[nodiscard]] const InstanceState& GetTopology() const noexcept
    {
        return topology_;
    }

    void ReadState(
        const StatePath& path,
        StateResponseEntries& snapshot) const;

    StateWriteStatus WriteState(
        const StateEntry& entry,
        StateResponseEntries& applied);

    [[nodiscard]] bool CanWrite(const StateEntry& entry) const;

private:
    [[nodiscard]] ApplyResult WriteGainState(
        const StatePath& path,
        const StateValue& value,
        dsp::GainState& state) const;

    [[nodiscard]] ApplyResult WriteFilterState(
        const StatePath& path,
        const StateValue& value,
        dsp::FilterState& state) const;

    [[nodiscard]] ApplyResult WriteSaturatorState(
        const StatePath& path,
        const StateValue& value);

    [[nodiscard]] ApplyResult WriteCompressorState(
        const StatePath& path,
        const StateValue& value);

    [[nodiscard]] ApplyResult WriteEqualizerState(
        const StatePath& path,
        const StateValue& value);

    InstanceState& topology_;
    ChainState chain_;
};

} // namespace consolidator::core
