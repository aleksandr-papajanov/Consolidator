#pragma once

#include <type_traits>
#include <utility>

#include "Core/Domain/State/StateStore.h"

namespace consolidator::core::detail
{

template <typename T>
ApplyResult ApplyParameter(
    const StatePath& path,
    const StateValue& value,
    dsp::ParameterState<T>& parameter)
{
    if (!path.parameterId || *path.parameterId != parameter.id)
    {
        return ApplyResult::NotHandled;
    }

    if (!std::holds_alternative<T>(value))
    {
        return ApplyResult::Rejected;
    }

    return parameter.Apply(
               path,
               dsp::ParameterVariant{std::get<T>(value)})
        ? ApplyResult::Applied
        : ApplyResult::Unchanged;
}

template <typename T>
void AppendParameter(
    const StatePath& query,
    StateResponseEntries& snapshot,
    StatePath path,
    const dsp::ParameterState<T>& parameter)
{
    path.field = StateField::DspParameter;
    path.instanceId = query.instanceId;
    if (!query.Matches(path))
    {
        return;
    }

    StateEntry entry{path, StateValue{parameter.value}};
    if constexpr (!std::is_same_v<T, bool>)
    {
        entry.physicalMinimum = dsp::ParameterVariant{parameter.minimum};
        entry.physicalMaximum = dsp::ParameterVariant{parameter.maximum};
        entry.minimum = entry.physicalMinimum;
        entry.maximum = entry.physicalMaximum;
    }
    (void)snapshot.TryAppend(std::move(entry));
}

} // namespace consolidator::core::detail
