#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <optional>

#include "Core/Parameters/ParameterValue.h"
#include "Core/State/StateProtocol.h"

namespace consolidator::core
{

struct DspUpdate
{
    StatePath path;
    dsp::ParameterValue value;
    std::uint64_t revision = 0;
};

struct DspStateBatch
{
    static constexpr std::size_t kMaximumUpdates = 512;

    std::array<DspUpdate, kMaximumUpdates> updates{};
    std::size_t count = 0;
    std::uint64_t revision = 0;
};

class DspUpdateMailbox final
{
public:
    static constexpr std::size_t kMaximumPathCount = 512;
    static constexpr std::size_t kSlotCount = kMaximumPathCount;
    static_assert(
        DspStateBatch::kMaximumUpdates >= kMaximumPathCount,
        "DspStateBatch must hold every published mailbox slot");

    // Publish/RegisterPath are coordinator-thread operations. The mailbox has
    // exactly one producer and one audio-thread consumer.

    void RegisterPath(const StatePath& path);
    void Publish(const DspUpdate& update);

    [[nodiscard]] bool ConsumeLatest(DspStateBatch& batch) noexcept;

private:
    struct Slot
    {
        bool used = false;
        StatePath path;
        std::atomic<std::uint64_t> packedValue{0};
        std::atomic<std::uint64_t> revision{0};
        std::atomic<std::uint64_t> sequence{0};
    };

    [[nodiscard]] static std::uint64_t PackValue(
        const dsp::ParameterValue& value) noexcept;
    [[nodiscard]] static std::optional<dsp::ParameterValue> UnpackValue(
        std::uint64_t packedValue) noexcept;
    [[nodiscard]] Slot* FindSlot(const StatePath& path) noexcept;

    std::array<Slot, kSlotCount> slots_;
    std::array<std::uint64_t, kSlotCount> consumedRevisions_{};
};

static_assert(
    DspStateBatch::kMaximumUpdates >=
        DspUpdateMailbox::kMaximumPathCount,
    "DspStateBatch capacity must cover every mailbox path");

} // namespace consolidator::core
