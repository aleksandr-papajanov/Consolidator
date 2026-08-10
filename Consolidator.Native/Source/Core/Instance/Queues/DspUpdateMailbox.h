#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "Core/Queues/LatestValueMailbox.h"
#include "Core/Domain/ParameterVariant.h"
#include "Core/Domain/State/StatePath.h"

namespace consolidator::core
{

struct DspUpdate
{
    StatePath path;
    dsp::ParameterVariant value;
    std::uint64_t revision = 0;
};

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

class DspUpdateMailbox final
{
public:
    static constexpr std::size_t kMaximumPathCount = 512;
    using ValueMailbox = LatestValueMailbox<
        StatePath,
        dsp::ParameterVariant,
        kMaximumPathCount>;

    static_assert(DspStateBatch::kMaximumUpdates >= kMaximumPathCount);

    void RegisterPath(const StatePath& path);
    void Publish(const DspUpdate& update);

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
