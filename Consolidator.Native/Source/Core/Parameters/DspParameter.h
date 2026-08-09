#pragma once

#include <algorithm>
#include <variant>

#include "Core/Parameters/ParameterRoute.h"
#include "Core/Parameters/ParameterValue.h"

namespace consolidator::dsp
{

template <typename T>
struct DspParameter
{
    ParameterId id;
    T value;
    T minimum;
    T maximum;

    constexpr operator const T&() const noexcept { return value; }

    constexpr DspParameter& operator=(T updatedValue) noexcept
    {
        value = std::clamp(updatedValue, minimum, maximum);
        return *this;
    }

    [[nodiscard]] bool Apply(const ParameterRoute& route, const ParameterValue& updatedValue) noexcept
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
struct DspParameter<bool>
{
    ParameterId id;
    bool value;

    constexpr operator bool() const noexcept { return value; }
    constexpr DspParameter& operator=(bool updatedValue) noexcept { value = updatedValue; return *this; }

    [[nodiscard]] bool Apply(const ParameterRoute& route, const ParameterValue& updatedValue) noexcept
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
