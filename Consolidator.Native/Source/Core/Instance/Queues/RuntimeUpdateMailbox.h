#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

#include "Core/Queues/LatestValueMailbox.h"
#include "Core/Instance/Queues/ParameterUpdates.h"
#include "Core/Instance/Queues/RuntimeControlUpdates.h"

namespace consolidator::core
{

template <>
struct LatestValueMailboxCodec<bool>
{
    using Storage = std::uint64_t;

    [[nodiscard]] static Storage Pack(bool value) noexcept
    {
        return value ? 1U : 0U;
    }

    [[nodiscard]] static std::optional<bool> Unpack(Storage value) noexcept
    {
        return value != 0;
    }
};

template <>
struct LatestValueMailboxCodec<dsp::ParameterVariant>
{
    using Storage = std::uint64_t;

    [[nodiscard]] static Storage Pack(
        const dsp::ParameterVariant& value) noexcept;

    [[nodiscard]] static std::optional<dsp::ParameterVariant> Unpack(
        Storage packedValue) noexcept;
};

using ParameterUpdateMailbox = LatestValueMailbox<
    StatePath,
    dsp::ParameterVariant,
    ParameterUpdateBatch::kMaximumUpdates>;
using RuntimeControlMailbox = LatestValueMailbox<
    RuntimeControlKey,
    bool,
    RuntimeControlBatch::kMaximumUpdates>;

// Owns the coalescing parameter and runtime-control snapshots delivered at block start.
class RuntimeUpdateMailbox final
{
public:
    static constexpr std::size_t kMaximumPathCount = 512;

    void RegisterPath(const StatePath& path);
    void EnqueueParameters(std::span<const ParameterUpdate> updates);
    [[nodiscard]] bool ConsumeLatest(ParameterUpdateBatch& batch) noexcept;

    void RegisterControlPath(
        const StatePath& path,
        RuntimeProperty property);
    void EnqueueRuntimeControls(std::span<const RuntimeControlUpdate> updates);
    [[nodiscard]] bool ConsumeControlLatest(
        RuntimeControlBatch& batch) noexcept;

private:
    ParameterUpdateMailbox parameterUpdates_;
    std::array<ParameterUpdateMailbox::Update, ParameterUpdateBatch::kMaximumUpdates>
        parameterSnapshot_;
    RuntimeControlMailbox controlUpdates_;
    std::array<RuntimeControlMailbox::Update, RuntimeControlBatch::kMaximumUpdates>
        controlSnapshot_;
    std::uint64_t nextParameterRevision_ = 0;
    std::uint64_t nextControlRevision_ = 0;
};

} // namespace consolidator::core
