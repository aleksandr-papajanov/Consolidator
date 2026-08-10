#pragma once

#include <algorithm>
#include <variant>

#include "Core/Domain/State/StateProtocol.h"
#include "Core/Domain/ParameterVariant.h"

namespace consolidator::dsp
{

template <typename T>
struct ParameterState
{
    ParameterId id;
    T value;
    T minimum;
    T maximum;

    constexpr operator const T&() const noexcept { return value; }

    constexpr ParameterState& operator=(T updatedValue) noexcept
    {
        value = std::clamp(updatedValue, minimum, maximum);
        return *this;
    }

    [[nodiscard]] bool Apply(const core::StatePath& route, const ParameterVariant& updatedValue) noexcept
    {
        if (route.GetParameterId() != id)
        {
            return false;
        }

        const auto* valueFromRoute = std::get_if<T>(&updatedValue);
        if (valueFromRoute == nullptr)
        {
            return false;
        }

        const T clampedValue = std::clamp(*valueFromRoute, minimum, maximum);
        if (value == clampedValue)
        {
            return false;
        }

        value = clampedValue;
        return true;
    }
};

template <>
struct ParameterState<bool>
{
    ParameterId id;
    bool value;

    constexpr operator bool() const noexcept { return value; }
    constexpr ParameterState& operator=(bool updatedValue) noexcept { value = updatedValue; return *this; }

    [[nodiscard]] bool Apply(const core::StatePath& route, const ParameterVariant& updatedValue) noexcept
    {
        if (route.GetParameterId() != id)
        {
            return false;
        }

        const auto* valueFromRoute = std::get_if<bool>(&updatedValue);
        if (valueFromRoute == nullptr || value == *valueFromRoute)
        {
            return false;
        }

        value = *valueFromRoute;
        return true;
    }
};

} // namespace consolidator::dsp
