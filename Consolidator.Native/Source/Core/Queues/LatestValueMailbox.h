#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <optional>
#include <span>
#include <utility>

namespace consolidator::core
{

template <typename Value>
struct LatestValueMailboxCodec;

// Single-producer/single-consumer mailbox that coalesces each key to its latest value.
template <typename Key, typename Value, std::size_t Capacity>
class LatestValueMailbox final
{
public:
    using Codec = LatestValueMailboxCodec<Value>;
    using Storage = typename Codec::Storage;

    struct Update
    {
        Key key;
        Value value;
        std::uint64_t revision = 0;
    };

    static_assert(Capacity > 0);
    static_assert(sizeof(Storage) <= sizeof(std::uint64_t));

    // Register and Publish have exactly one producer. ConsumeLatest has one
    // consumer. Registration must finish before the consumer starts.
    // Adds a fixed slot; all keys must be registered before consumption begins.
    void Register(const Key& key)
    {
        if (FindSlot(key) != nullptr)
        {
            return;
        }

        for (auto& slot : slots_)
        {
            if (!slot.used)
            {
                slot.key = key;
                slot.used = true;
                return;
            }
        }

        assert(false && "LatestValueMailbox capacity exhausted");
        std::terminate();
    }

    // Publishes a packed value using a seqlock so readers get a consistent snapshot.
    void Publish(
        const Key& key,
        const Value& value,
        std::uint64_t revision)
    {
        auto* slot = FindSlot(key);
        assert(slot != nullptr);
        if (slot == nullptr)
        {
            std::terminate();
        }

        const auto sequence = slot->sequence.fetch_add(
            1,
            std::memory_order_acq_rel);
        if ((sequence & 1U) != 0)
        {
            std::terminate();
        }

        slot->packedValue.store(
            Codec::Pack(value),
            std::memory_order_relaxed);
        slot->revision.store(revision, std::memory_order_relaxed);
        slot->sequence.store(sequence + 2, std::memory_order_release);
    }

    // Collects values newer than the last consumed revision without blocking.
    [[nodiscard]] bool ConsumeLatest(
        std::span<Update> output,
        std::size_t& outputCount,
        std::uint64_t& batchRevision) noexcept
    {
        std::size_t count = 0;
        batchRevision = 0;

        for (std::size_t slotIndex = 0; slotIndex < slots_.size(); ++slotIndex)
        {
            auto& slot = slots_[slotIndex];
            if (!slot.used)
            {
                continue;
            }

            const auto sequenceBefore =
                slot.sequence.load(std::memory_order_acquire);
            if ((sequenceBefore & 1U) != 0)
            {
                continue;
            }

            const auto packedValue =
                slot.packedValue.load(std::memory_order_relaxed);
            const auto revision =
                slot.revision.load(std::memory_order_relaxed);
            const auto sequenceAfter =
                slot.sequence.load(std::memory_order_acquire);
            if (sequenceBefore != sequenceAfter ||
                (sequenceAfter & 1U) != 0 ||
                revision <= consumedRevisions_[slotIndex])
            {
                continue;
            }

            const auto value = Codec::Unpack(packedValue);
            assert(value.has_value());
            if (!value)
            {
                std::terminate();
            }

            if (count == output.size())
            {
                assert(false && "LatestValueMailbox output capacity exhausted");
                std::terminate();
            }

            output[count++] = Update{slot.key, *value, revision};
            batchRevision = std::max(batchRevision, revision);
            consumedRevisions_[slotIndex] = revision;
        }

        outputCount = count;
        return count != 0;
    }

private:
    struct Slot
    {
        bool used = false;
        Key key;
        std::atomic<std::uint64_t> packedValue{0};
        std::atomic<std::uint64_t> revision{0};
        std::atomic<std::uint64_t> sequence{0};
    };

    [[nodiscard]] Slot* FindSlot(const Key& key) noexcept
    {
        for (auto& slot : slots_)
        {
            if (slot.used && slot.key == key)
            {
                return &slot;
            }
        }
        return nullptr;
    }

    std::array<Slot, Capacity> slots_;
    std::array<std::uint64_t, Capacity> consumedRevisions_{};
};

} // namespace consolidator::core
