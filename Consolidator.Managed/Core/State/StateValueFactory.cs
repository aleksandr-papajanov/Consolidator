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
    private readonly DspStateChangeTracker _dspChanges;
    private readonly IActivityStatusSink _activityStatusSink;

    internal StateValueFactory(
        StateRegistry<InstanceId> registry,
        StatePeerObserver peerObserver,
        StateValueMetadataRegistry metadata,
        IStateChangeSink stateChangeSink,
        DspStateChangeTracker dspChanges,
        IActivityStatusSink activityStatusSink)
    {
        _registry = registry;
        _peerObserver = peerObserver;
        _metadata = metadata;
        _stateChangeSink = stateChangeSink;
        _dspChanges = dspChanges;
        _activityStatusSink = activityStatusSink;
    }

    public IActivityStatusSink ActivityStatusSink => _activityStatusSink;

    public StateValue<TValue> Create<TValue>(
        StateValueCreationContext context,
        StateValueDefinition<TValue> definition,
        params IStateValueObserver<TValue>[] observers)
    {
        ArgumentNullException.ThrowIfNull(context);
        ArgumentNullException.ThrowIfNull(definition);
        ValidatePath(context.Path);
        return CreateValue(
            context,
            definition.DefaultValue,
            definition.PhysicalRange,
            definition.RegisterInHistory,
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

    private StateValue<TValue> CreateValue<TValue>(
        StateValueCreationContext context,
        TValue initialValue,
        FloatRange? physicalRange,
        bool registerInHistory,
        IReadOnlyList<IStateValueObserver<TValue>> observers)
    {
        ArgumentNullException.ThrowIfNull(context.Path);
        ArgumentNullException.ThrowIfNull(observers);

        var stateChangeObserver = new StateChangeObserver<TValue>(
            context.InstanceId,
            context.Path,
            context.Ownership,
            _stateChangeSink);
        var peerObserver = _peerObserver.Create<TValue>(
            context.InstanceId,
            context.Path,
            context.Scope,
            physicalRange,
            stateChangeObserver.EffectiveRangeChanged);
        var valueObservers = observers
            .Append(peerObserver)
            .Append(_metadata.Observe<TValue>(
                context.InstanceId,
                context.Path,
                physicalRange,
                peerObserver.GetEffectiveRange))
            .Append(stateChangeObserver)
            .Append(new DspStateObserver<TValue>(
                _dspChanges,
                context.InstanceId))
            .ToArray();
        return registerInHistory
            ? _registry.CreateValue(context.InstanceId, context.Path, initialValue, valueObservers)
            : _registry.CreateValueWithoutHistory(context.InstanceId, context.Path, initialValue, valueObservers);
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
