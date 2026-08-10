#include "Core/Instance/Queues/DspUpdateMailbox.h"

#include <bit>
#include <type_traits>

namespace consolidator::core
{

std::uint64_t LatestValueMailboxCodec<dsp::ParameterVariant>::Pack(
    const dsp::ParameterVariant& value) noexcept
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

std::optional<dsp::ParameterVariant>
LatestValueMailboxCodec<dsp::ParameterVariant>::Unpack(
    std::uint64_t packedValue) noexcept
{
    const auto tag = packedValue >> 56;
    const auto payload = static_cast<std::uint32_t>(packedValue);
    switch (tag)
    {
    case 1:
        return dsp::ParameterVariant{payload != 0};
    case 2:
        return dsp::ParameterVariant{
            static_cast<std::int32_t>(payload)};
    case 3:
        return dsp::ParameterVariant{
            std::bit_cast<float>(payload)};
    default:
        return std::nullopt;
    }
}

void DspUpdateMailbox::RegisterPath(const StatePath& path)
{
    mailbox_.Register(path);
}

void DspUpdateMailbox::Publish(const DspUpdate& update)
{
    mailbox_.Publish(update.path, update.value, update.revision);
}

bool DspUpdateMailbox::ConsumeLatest(DspStateBatch& batch) noexcept
{
    batch.count = 0;
    if (!mailbox_.ConsumeLatest(
            std::span<ValueMailbox::Update>{
                updates_.data(),
                updates_.size()},
            batch.count,
            batch.revision))
    {
        return false;
    }

    // Convert the reusable mailbox records to the public DSP batch without
    // exposing the generic mailbox implementation to the DSP chain.
    const auto updateCount = batch.count;
    batch.count = 0;
    for (std::size_t index = 0; index < updateCount; ++index)
    {
        batch.updates[batch.count++] = DspUpdate{
            updates_[index].key,
            updates_[index].value,
            updates_[index].revision};
    }
    return batch.count != 0;
}

} // namespace consolidator::core
