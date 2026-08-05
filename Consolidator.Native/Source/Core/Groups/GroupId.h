#pragma once

#include <cstddef>
#include <functional>

namespace consolidator::core
{

class GroupId
{
public:
    using ValueType = std::size_t;

    constexpr explicit GroupId(ValueType value) noexcept
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

constexpr bool operator==(GroupId lhs, GroupId rhs) noexcept
{
    return lhs.GetValue() == rhs.GetValue();
}

constexpr bool operator!=(GroupId lhs, GroupId rhs) noexcept
{
    return !(lhs == rhs);
}

} // namespace consolidator::core

template <>
struct std::hash<consolidator::core::GroupId>
{
    std::size_t operator()(const consolidator::core::GroupId& id) const noexcept
    {
        return std::hash<consolidator::core::GroupId::ValueType>{}(id.GetValue());
    }
};
