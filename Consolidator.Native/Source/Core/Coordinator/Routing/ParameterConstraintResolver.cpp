#include "Core/Coordinator/Routing/ParameterConstraintResolver.h"

#include <algorithm>
#include <cmath>
#include <type_traits>
#include <vector>

#include "Core/Instance/ConsolidatorInstance.h"

namespace consolidator::core
{

namespace
{

bool IsNumeric(const StateEntry& entry) noexcept
{
    return entry.physicalMinimum && entry.physicalMaximum &&
        std::holds_alternative<float>(*entry.physicalMinimum) &&
        std::holds_alternative<float>(*entry.physicalMaximum);
}

bool IsInRange(const StateEntry& target, double delta) noexcept
{
    if (!IsNumeric(target) || !std::holds_alternative<float>(target.value))
    {
        return true;
    }

    const auto minimum = std::get<float>(*target.physicalMinimum);
    const auto maximum = std::get<float>(*target.physicalMaximum);
    const auto value = std::get<float>(target.value);
    return value + delta >= minimum && value + delta <= maximum;
}

} // namespace

bool ParameterConstraintResolver::ReadParameter(
    InstanceId instanceId,
    const StatePath& path,
    StateEntry& result) const
{
    const auto* instance = registry_.FindInstance(instanceId);
    if (instance == nullptr)
    {
        return false;
    }

    StateResponseEntries stateStoreEntries;
    instance->GetStateStore().ReadState(path, stateStoreEntries);
    for (std::size_t index = 0; index < stateStoreEntries.size; ++index)
    {
        if (stateStoreEntries.entries[index].path.Matches(path))
        {
            result = stateStoreEntries.entries[index];
            return true;
        }
    }

    return false;
}

bool ParameterConstraintResolver::Validate(
    InstanceId sourceInstanceId,
    const StateEntry& requested) const
{
    if (requested.path.field != StateField::DspParameter)
    {
        return true;
    }

    StateEntry source;
    if (!ReadParameter(sourceInstanceId, requested.path, source))
    {
        return false;
    }
    if (!std::holds_alternative<float>(requested.value) ||
        !std::holds_alternative<float>(source.value))
    {
        return true;
    }

    const auto delta = static_cast<double>(std::get<float>(requested.value)) -
        static_cast<double>(std::get<float>(source.value));
    auto targets = stateRouter_.ResolveWriteTargets(sourceInstanceId, requested.path);
    if (targets.empty())
    {
        return IsInRange(source, delta);
    }

    for (const auto& target : targets)
    {
        StateEntry targetEntry;
        auto targetPath = StateRouter::IsBankOwned(requested.path)
            ? StateRouter::ForBank(requested, target.bankId).path
            : requested.path;
        targetPath.instanceId = target.instanceId;
        if (!ReadParameter(target.instanceId, targetPath, targetEntry) ||
            !IsInRange(targetEntry, delta))
        {
            return false;
        }
    }
    return true;
}

std::optional<StateEntry> ParameterConstraintResolver::TranslateForTarget(
    InstanceId sourceInstanceId,
    const StateEntry& requested,
    const BankAddress& target) const
{
    auto translated = requested;
    if (requested.path.field != StateField::DspParameter ||
        !std::holds_alternative<float>(requested.value))
    {
        return translated;
    }

    StateEntry source;
    StateEntry targetEntry;
    if (!ReadParameter(sourceInstanceId, requested.path, source))
    {
        return std::nullopt;
    }

    auto targetPath = StateRouter::IsBankOwned(requested.path)
        ? StateRouter::ForBank(requested, target.bankId).path
        : requested.path;
    targetPath.instanceId = target.instanceId;
    if (!ReadParameter(target.instanceId, targetPath, targetEntry) ||
        !std::holds_alternative<float>(source.value) ||
        !std::holds_alternative<float>(targetEntry.value))
    {
        return std::nullopt;
    }

    const auto delta = std::get<float>(requested.value) -
        std::get<float>(source.value);
    translated.value = StateValue{
        std::get<float>(targetEntry.value) + delta};
    return translated;
}

void ParameterConstraintResolver::Enrich(
    InstanceId sourceInstanceId,
    StateEntry& entry) const
{
    if (entry.path.field != StateField::DspParameter)
    {
        return;
    }

    auto targets = stateRouter_.ResolveConstraintDependencies(sourceInstanceId, entry.path);
    if (targets.empty())
    {
        if (entry.physicalMinimum && entry.physicalMaximum &&
            std::holds_alternative<float>(entry.value))
        {
            entry.minimum = entry.physicalMinimum;
            entry.maximum = entry.physicalMaximum;
        }
        return;
    }

    bool initialized = false;
    float minimumDelta = 0.0F;
    float maximumDelta = 0.0F;
    for (const auto& target : targets)
    {
        StateEntry targetEntry;
        auto targetPath = StateRouter::IsBankOwned(entry.path)
            ? StateRouter::ForBank(entry, target.bankId).path
            : entry.path;
        targetPath.instanceId = target.instanceId;
        if (!ReadParameter(target.instanceId, targetPath, targetEntry) ||
            !std::holds_alternative<float>(targetEntry.value) ||
            !targetEntry.physicalMinimum || !targetEntry.physicalMaximum ||
            !std::holds_alternative<float>(*targetEntry.physicalMinimum) ||
            !std::holds_alternative<float>(*targetEntry.physicalMaximum))
        {
            return;
        }

        const auto value = std::get<float>(targetEntry.value);
        const auto candidateMinimum = std::get<float>(*targetEntry.physicalMinimum) - value;
        const auto candidateMaximum = std::get<float>(*targetEntry.physicalMaximum) - value;
        if (!initialized)
        {
            minimumDelta = candidateMinimum;
            maximumDelta = candidateMaximum;
            initialized = true;
        }
        else
        {
            minimumDelta = std::max(minimumDelta, candidateMinimum);
            maximumDelta = std::min(maximumDelta, candidateMaximum);
        }
    }

    if (initialized)
    {
        const auto sourceValue = std::get<float>(entry.value);
        entry.minimum = dsp::ParameterValue{sourceValue + minimumDelta};
        entry.maximum = dsp::ParameterValue{sourceValue + maximumDelta};
    }
}

} // namespace consolidator::core
