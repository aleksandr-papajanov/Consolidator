using Consolidator.Managed.Core.Settings;
using Consolidator.Managed.Core.State.Observers;
using Consolidator.Managed.State;
using Consolidator.Managed.State.Observers;

namespace Consolidator.Managed.Core.State.Models;

public sealed class FilterState
{
    internal FilterState(
        InstanceId instanceId,
        StatePath path,
        StateValueFactory values,
        bool bankOwned,
        Action<bool> bypassProjection,
        FilterDefinition definition,
        ActivityObserver? activity = null,
        int bankId = -1,
        int filterId = -1)
    {
        ArgumentNullException.ThrowIfNull(definition);
        Definition = definition;
        FrequencyHz = Definition.Frequency is { } frequency
            ? CreateValue(
            instanceId,
            path.Append(StateNodeIds.Frequency),
            values,
            bankOwned,
            frequency.DefaultValue,
            frequency.Range)
            : null;
        Q = Definition.Q is { } q
            ? CreateValue(
            instanceId,
            path.Append(StateNodeIds.Q),
            values,
            bankOwned,
            q.DefaultValue,
            q.Range)
            : null;
        GainDb = CreateValue(
            instanceId,
            path.Append(StateNodeIds.Gain),
            values,
            bankOwned,
            Definition.Gain.DefaultValue,
            Definition.Gain.Range,
            activity is null
                ? Array.Empty<IStateValueObserver<float>>()
                : [activity.ObserveFilterGain(bankId, filterId)]);
        Bypass = CreateValue(
            instanceId,
            path.Append(StateNodeIds.Bypass),
            values,
            bankOwned,
            false,
            activity is null
                ? [new StateProjectionObserver<bool>(bypassProjection)]
                : [
                    new StateProjectionObserver<bool>(bypassProjection),
                    activity.ObserveFilterBypass(bankId, filterId)
                ]);
        Solo = CreateValue(
            instanceId,
            path.Append(StateNodeIds.Solo),
            values,
            bankOwned,
            false);
    }

    public StateValue<float>? FrequencyHz { get; }

    public FilterDefinition Definition { get; }

    public StateValue<float>? Q { get; }

    public StateValue<float> GainDb { get; }

    public StateValue<bool> Bypass { get; }

    public StateValue<bool> Solo { get; }

    private static StateValue<TValue> CreateValue<TValue>(
        InstanceId instanceId,
        StatePath path,
        StateValueFactory values,
        bool bankOwned,
        TValue initialValue,
        params IStateValueObserver<TValue>[] observers)
    {
        return bankOwned
            ? values.CreateBankValue(
                instanceId,
                path,
                initialValue,
                StateValueEditMode.CopyValue,
                observers: observers)
            : values.CreateValue(
                instanceId,
                path,
                initialValue,
                StateValueEditMode.CopyValue,
                observers: observers);
    }

    private static StateValue<float> CreateValue(
        InstanceId instanceId,
        StatePath path,
        StateValueFactory values,
        bool bankOwned,
        float initialValue,
        FloatRange physicalRange,
        params IStateValueObserver<float>[] observers)
    {
        return bankOwned
            ? values.CreateBankValue(
                instanceId,
                path,
                initialValue,
                StateValueEditMode.ApplyDelta,
                physicalRange,
                observers: observers)
            : values.CreateValue(
                instanceId,
                path,
                initialValue,
                StateValueEditMode.ApplyDelta,
                physicalRange,
                observers: observers);
    }
}
