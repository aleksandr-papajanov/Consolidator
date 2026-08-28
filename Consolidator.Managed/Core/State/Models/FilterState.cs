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
        EqualizerBankEffectObserver? effectObserver = null,
        int filterId = -1)
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
            DspParameterRanges.FilterGainDb,
            effectObserver is null
                ? Array.Empty<IStateValueObserver<float>>()
                : [effectObserver.ObserveFilterGain(filterId)]);
        Bypass = CreateValue(
            instanceId,
            path.Append(StateNodeIds.Bypass),
            values,
            bankOwned,
            false,
            effectObserver is null
                ? [new StateProjectionObserver<bool>(bypassProjection)]
                : [
                    new StateProjectionObserver<bool>(bypassProjection),
                    effectObserver.ObserveFilterBypass(filterId)
                ]);
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
