#pragma once

#include <type_traits>
#include <variant>

#include "Dsp/Parameters/ParameterChange.h"

#ifdef CONSOLIDATOR_DEV
#include <cassert>
#endif

namespace consolidator::dsp
{

template <typename T>
const T* TryGetValue(const ParameterChange& change) noexcept
{
    const auto* value = std::get_if<T>(&change.value);
#ifdef CONSOLIDATOR_DEV
    assert(value != nullptr && "ParameterChange value type mismatch");
#endif
    return value;
}

} // namespace consolidator::dsp