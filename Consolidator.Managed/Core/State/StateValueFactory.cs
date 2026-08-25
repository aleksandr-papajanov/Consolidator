using Consolidator.Managed.Analyzer;
using Consolidator.Managed.Core.Dsp;
using Consolidator.Managed.Core.Services.Abstractions;
using Consolidator.Managed.Core.State.Observers;
using Consolidator.Managed.State;
using Consolidator.Managed.State.Observers;

namespace Consolidator.Managed.Core.State;

public sealed class StateValueFactory
{
    private readonly StateRegistry<InstanceId> _registry;
    private readonly StatePeerObserver _peerObserver;
    private readonly StateValueMetadataRegistry _metadata;
    private readonly IStateChangeSink _stateChangeSink;
    private readonly AnalyzerRegistry _analyzerRegistry;
    private readonly DspStateChangeTracker _dspChanges;

    internal StateValueFactory(
        StateRegistry<InstanceId> registry,
        StatePeerObserver peerObserver,
        StateValueMetadataRegistry metadata,
        IStateChangeSink stateChangeSink,
        AnalyzerRegistry analyzerRegistry,
        DspStateChangeTracker dspChanges)
    {
        _registry = registry;
        _peerObserver = peerObserver;
        _metadata = metadata;
        _stateChangeSink = stateChangeSink;
        _analyzerRegistry = analyzerRegistry;
        _dspChanges = dspChanges;
    }

    public StateValue<TValue> CreateValue<TValue>(
        InstanceId instanceId,
        StatePath path,
        TValue initialValue,
        StateValueEditMode editMode,
        FloatRange? physicalRange = null,
        params IStateValueObserver<TValue>[] observers)
    {
        ValidatePath(path);
        var scope = path.Nodes[0].Equals(StateNodeIds.Instance)
            ? StateValueEditScope.Local
            : StateValueEditScope.ConnectedInstances;
        return Create(
            instanceId,
            path,
            initialValue,
            scope,
            editMode,
            physicalRange,
            StateValueOwnership.InstanceOwned,
            observers);
    }

    public StateValue<TValue> CreateBankValue<TValue>(
        InstanceId instanceId,
        StatePath path,
        TValue initialValue,
        StateValueEditMode editMode,
        FloatRange? physicalRange = null,
        StateValueEditScope scope = StateValueEditScope.ConnectedInstances,
        params IStateValueObserver<TValue>[] observers)
    {
        ValidatePath(path);
        return Create(
            instanceId,
            path,
            initialValue,
            scope,
            editMode,
            physicalRange,
            StateValueOwnership.BankOwned,
            observers);
    }

    public void CreateTransient<TValue>(
        InstanceId instanceId,
        StatePath path,
        Func<TValue> read,
        Action<TValue> write)
    {
        ValidatePath(path);
        _registry.CreateTransient(instanceId, path, read, write);
    }

    private StateValue<TValue> Create<TValue>(
        InstanceId instanceId,
        StatePath path,
        TValue initialValue,
        StateValueEditScope scope,
        StateValueEditMode editMode,
        FloatRange? physicalRange,
        StateValueOwnership ownership,
        IReadOnlyList<IStateValueObserver<TValue>> observers)
    {
        ArgumentNullException.ThrowIfNull(path);
        ArgumentNullException.ThrowIfNull(observers);

        var stateChangeObserver = new StateChangeObserver<TValue>(
            instanceId,
            path,
            ownership,
            _stateChangeSink);
        var peerObserver = _peerObserver.Create<TValue>(
            instanceId,
            path,
            scope,
            editMode,
            physicalRange,
            stateChangeObserver.EffectiveRangeChanged);
        var valueObservers = observers
            .Append(peerObserver)
            .Append(_metadata.Observe<TValue>(
                instanceId,
                path,
                physicalRange,
                peerObserver.GetEffectiveRange))
            .Append(stateChangeObserver)
            .Append(new DspStateObserver<TValue>(
                _analyzerRegistry,
                _dspChanges,
                instanceId,
                path))
            .ToArray();
        return _registry.CreateValue(
            instanceId,
            path,
            initialValue,
            valueObservers);
    }

    private static void ValidatePath(StatePath path)
    {
        ArgumentNullException.ThrowIfNull(path);
        if (path.Depth == 0)
        {
            throw new ArgumentException(
                "A concrete state value requires a non-empty path.",
                nameof(path));
        }
    }
}
