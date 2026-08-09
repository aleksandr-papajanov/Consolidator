#include "Core/Instance/Queues/DspUpdateMailbox.h"

#include <algorithm>
#include <cassert>
#include <bit>
#include <exception>
#include <type_traits>

namespace consolidator::core
{

void DspUpdateMailbox::RegisterPath(const StatePath& path)
{
    if (FindSlot(path) != nullptr)
    {
        return;
    }

    for (auto& slot : slots_)
    {
        if (!slot.used)
        {
            slot.path = path;
            slot.used = true;
            return;
        }
    }

    assert(false && "DspUpdateMailbox path capacity exhausted");
    std::terminate();
}

void DspUpdateMailbox::Publish(const DspUpdate& update)
{
    auto* slot = FindSlot(update.path);
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
        PackValue(update.value),
        std::memory_order_relaxed);
    slot->revision.store(update.revision, std::memory_order_relaxed);
    slot->sequence.store(sequence + 2, std::memory_order_release);
}

bool DspUpdateMailbox::ConsumeLatest(DspStateBatch& batch) noexcept
{
    batch.count = 0;
    batch.revision = 0;

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
        const auto packedValue = slot.packedValue.load(std::memory_order_relaxed);
        const auto revision = slot.revision.load(std::memory_order_relaxed);
        const auto sequenceAfter =
            slot.sequence.load(std::memory_order_acquire);
        if (sequenceBefore != sequenceAfter ||
            (sequenceAfter & 1U) != 0 ||
            revision <= consumedRevisions_[slotIndex])
        {
            continue;
        }
        const auto value = UnpackValue(packedValue);
        assert(value.has_value());
        if (!value)
        {
            std::terminate();
        }
        if (batch.count < batch.updates.size())
        {
            batch.updates[batch.count++] = DspUpdate{
                slot.path,
                *value,
                revision};
            batch.revision = std::max(batch.revision, revision);
            consumedRevisions_[slotIndex] = revision;
        }
        else
        {
            assert(false && "DspUpdateMailbox batch capacity exhausted");
            std::terminate();
        }
    }

    return batch.count != 0;
}

std::uint64_t DspUpdateMailbox::PackValue(
    const dsp::ParameterValue& value) noexcept
{
    return std::visit(
        [](const auto& typedValue)
        {
            using ValueType = std::decay_t<decltype(typedValue)>;
            if constexpr (std::is_same_v<ValueType, bool>)
            {
                return (std::uint64_t{1} << 56) |
                    static_cast<std::uint64_t>(typedValue);
            }
            else if constexpr (std::is_same_v<ValueType, std::int32_t>)
            {
                return (std::uint64_t{2} << 56) |
                    static_cast<std::uint64_t>(
                        static_cast<std::uint32_t>(typedValue));
            }
            else
            {
                return (std::uint64_t{3} << 56) |
                    static_cast<std::uint64_t>(
                        std::bit_cast<std::uint32_t>(typedValue));
            }
        },
        value);
}

std::optional<dsp::ParameterValue> DspUpdateMailbox::UnpackValue(
    std::uint64_t packedValue) noexcept
{
    const auto tag = packedValue >> 56;
    const auto payload = static_cast<std::uint32_t>(packedValue);
    switch (tag)
    {
    case 1:
        return dsp::ParameterValue{payload != 0};
    case 2:
        return dsp::ParameterValue{
            static_cast<std::int32_t>(payload)};
    case 3:
        return dsp::ParameterValue{
            std::bit_cast<float>(payload)};
    default:
        return std::nullopt;
    }
}

DspUpdateMailbox::Slot* DspUpdateMailbox::FindSlot(
    const StatePath& path) noexcept
{
    for (auto& slot : slots_)
    {
        if (slot.used && slot.path == path)
        {
            return &slot;
        }
    }
    return nullptr;
}

} // namespace consolidator::core
