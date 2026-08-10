#pragma once

#include <cstddef>
#include <functional>

namespace consolidator::core
{

// Strongly typed identifier for a live processor instance.
class InstanceId
{
public:
    using ValueType = std::size_t;

    constexpr explicit InstanceId(ValueType value) noexcept
        : value_(value)
    {
    }

    [[nodiscard]] constexpr ValueType GetValue() const noexcept
    {
        return value_;
    }

private:
    ValueType value_;
};

constexpr bool operator==(InstanceId lhs, InstanceId rhs) noexcept
{
    return lhs.GetValue() == rhs.GetValue();
}

constexpr bool operator!=(InstanceId lhs, InstanceId rhs) noexcept
{
    return !(lhs == rhs);
}

} // namespace consolidator::core

template <>
struct std::hash<consolidator::core::InstanceId>
{
    std::size_t operator()(const consolidator::core::InstanceId& id) const noexcept
    {
        return std::hash<consolidator::core::InstanceId::ValueType>{}(id.GetValue());
    }
};
