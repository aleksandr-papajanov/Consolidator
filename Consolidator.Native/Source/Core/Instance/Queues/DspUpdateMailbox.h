#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "Core/Queues/LatestValueMailbox.h"
#include "Core/Domain/ParameterVariant.h"
#include "Core/Domain/State/StatePath.h"

namespace consolidator::core
{

// A single coordinator-to-audio-thread runtime parameter update.
struct DspUpdate
{
    StatePath path;
    dsp::ParameterVariant value;
    std::uint64_t revision = 0;
};

// Fixed-capacity batch consumed by a DSP chain at the start of an audio block.
struct DspStateBatch
{
    static constexpr std::size_t kMaximumUpdates = 512;

    std::array<DspUpdate, kMaximumUpdates> updates{};
    std::size_t count = 0;
    std::uint64_t revision = 0;
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

// Coalesces registered DSP paths so the audio thread sees only their latest values.
class DspUpdateMailbox final
{
public:
    static constexpr std::size_t kMaximumPathCount = 512;
    using ValueMailbox = LatestValueMailbox<
        StatePath,
        dsp::ParameterVariant,
        kMaximumPathCount>;

    static_assert(DspStateBatch::kMaximumUpdates >= kMaximumPathCount);

    // Registration defines the fixed set of paths and must precede processing.
    void RegisterPath(const StatePath& path);
    // Publishes a value for a registered path from the coordinator thread.
    void Publish(const DspUpdate& update);

    // Copies a consistent, revision-ordered snapshot for the audio thread.
    [[nodiscard]] bool ConsumeLatest(DspStateBatch& batch) noexcept;

private:
    ValueMailbox mailbox_;
    std::array<typename ValueMailbox::Update, kMaximumPathCount> updates_;
};

static_assert(
    DspStateBatch::kMaximumUpdates >=
        DspUpdateMailbox::kMaximumPathCount,
    "DspStateBatch capacity must cover every mailbox path");

} // namespace consolidator::core
