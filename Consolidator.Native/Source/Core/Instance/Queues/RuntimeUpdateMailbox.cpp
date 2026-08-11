#include "Core/Instance/Queues/RuntimeUpdateMailbox.h"

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
                    static_cast<std::uint64_t>(static_cast<std::uint32_t>(typedValue));
            }
            else
            {
                return (std::uint64_t{3} << 56) |
                    static_cast<std::uint64_t>(std::bit_cast<std::uint32_t>(typedValue));
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
        return dsp::ParameterVariant{static_cast<std::int32_t>(payload)};
    case 3:
        return dsp::ParameterVariant{std::bit_cast<float>(payload)};
    default:
        return std::nullopt;
    }
}

void RuntimeUpdateMailbox::RegisterPath(const StatePath& path)
{
    parameterUpdates_.Register(path);
}

void RuntimeUpdateMailbox::EnqueueParameters(
    std::span<const ParameterUpdate> updates)
{
    for (const auto& input : updates)
    {
        auto update = input;
        update.revision = ++nextParameterRevision_;
        parameterUpdates_.Publish(update.path, update.value, update.revision);
    }
}

bool RuntimeUpdateMailbox::ConsumeLatest(ParameterUpdateBatch& batch) noexcept
{
    batch.count = 0;
    if (!parameterUpdates_.ConsumeLatest(
            std::span<ParameterUpdateMailbox::Update>{
                parameterSnapshot_.data(), parameterSnapshot_.size()},
            batch.count,
            batch.revision))
    {
        return false;
    }

    const auto updateCount = batch.count;
    batch.count = 0;
    for (std::size_t index = 0; index < updateCount; ++index)
    {
        batch.updates[batch.count++] = ParameterUpdate{
            parameterSnapshot_[index].key,
            parameterSnapshot_[index].value,
            parameterSnapshot_[index].revision};
    }
    return batch.count != 0;
}

void RuntimeUpdateMailbox::RegisterControlPath(
    const StatePath& path,
    RuntimeProperty property)
{
    controlUpdates_.Register(RuntimeControlKey{path, property});
}

void RuntimeUpdateMailbox::EnqueueRuntimeControls(
    std::span<const RuntimeControlUpdate> updates)
{
    for (const auto& input : updates)
    {
        auto update = input;
        update.revision = ++nextControlRevision_;
        controlUpdates_.Publish(
            RuntimeControlKey{update.target, update.property},
            update.value,
            update.revision);
    }
}

bool RuntimeUpdateMailbox::ConsumeControlLatest(
    RuntimeControlBatch& batch) noexcept
{
    batch.count = 0;
    if (!controlUpdates_.ConsumeLatest(
            std::span<RuntimeControlMailbox::Update>{
                controlSnapshot_.data(), controlSnapshot_.size()},
            batch.count,
            batch.revision))
    {
        return false;
    }

    const auto updateCount = batch.count;
    batch.count = 0;
    for (std::size_t index = 0; index < updateCount; ++index)
    {
        batch.updates[batch.count++] = RuntimeControlUpdate{
            controlSnapshot_[index].key.target,
            controlSnapshot_[index].key.property,
            controlSnapshot_[index].value,
            controlSnapshot_[index].revision};
    }
    return batch.count != 0;
}

} // namespace consolidator::core
