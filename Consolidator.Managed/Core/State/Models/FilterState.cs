using Consolidator.Managed.Core.Settings;
using Consolidator.Managed.Core.State.Observers;
using Consolidator.Managed.State;
using Consolidator.Managed.State.Observers;

namespace Consolidator.Managed.Core.State.Models;

public sealed class FilterState
{
    public FilterState(
        InstanceId instanceId,
        StatePath path,
        StateValueFactory values,
        bool bankOwned,
        Action<bool> bypassProjection)
    {
        FrequencyHz = CreateValue(
            instanceId,
            path.Append(StateNodeIds.Frequency),
            values,
            bankOwned,
            1000.0F,
            DspParameterRanges.FrequencyHz);
        Q = CreateValue(
            instanceId,
            path.Append(StateNodeIds.Q),
            values,
            bankOwned,
            1.0F,
            DspParameterRanges.Q);
        GainDb = CreateValue(
            instanceId,
            path.Append(StateNodeIds.Gain),
            values,
            bankOwned,
            0.0F,
            DspParameterRanges.FilterGainDb);
        Bypass = CreateValue(
            instanceId,
            path.Append(StateNodeIds.Bypass),
            values,
            bankOwned,
            false,
            new StateProjectionObserver<bool>(bypassProjection));
        Solo = CreateValue(
            instanceId,
            path.Append(StateNodeIds.Solo),
            values,
            bankOwned,
            false);
    }

    public StateValue<float> FrequencyHz { get; }

    public StateValue<float> Q { get; }

    public StateValue<float> GainDb { get; }

    public StateValue<bool> Bypass { get; }

    public StateValue<bool> Solo { get; }

    private static StateValue<TValue> CreateValue<TValue>(
        InstanceId instanceId,
        StatePath path,
        StateValueFactory values,
        bool bankOwned,
        TValue initialValue,
        IStateValueObserver<TValue>? observer = null)
    {
        IStateValueObserver<TValue>[] observers = observer is null
            ? Array.Empty<IStateValueObserver<TValue>>()
            : [observer];
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
        FloatRange physicalRange)
    {
        return bankOwned
            ? values.CreateBankValue(
                instanceId,
                path,
                initialValue,
                StateValueEditMode.ApplyDelta,
                physicalRange)
            : values.CreateValue(
                instanceId,
                path,
                initialValue,
                StateValueEditMode.ApplyDelta,
                physicalRange);
    }
}
