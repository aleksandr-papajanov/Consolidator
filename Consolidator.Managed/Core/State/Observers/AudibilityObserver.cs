using Consolidator.Managed.Core.Dsp;
using Consolidator.Managed.State;
using Consolidator.Managed.State.Observers;

namespace Consolidator.Managed.Core.State.Observers;

internal sealed class AudibilityObserver
{
    private readonly DspStateChangeTracker _dspChanges;
    private readonly Dictionary<InstanceId, InstanceEntry> _instances = new();
    private readonly object _lock = new();

    public AudibilityObserver(DspStateChangeTracker dspChanges)
    {
        _dspChanges = dspChanges;
    }

    public IStateValueObserver<bool> ObserveMute(
        InstanceId instanceId,
        DspRuntimeState runtime)
    {
        return new ValueObserver(this, instanceId, runtime, true);
    }

    public IStateValueObserver<bool> ObserveSolo(
        InstanceId instanceId,
        DspRuntimeState runtime)
    {
        return new ValueObserver(this, instanceId, runtime, false);
    }

    public void Refresh()
    {
        InstanceEntry[] instances;
        lock (_lock)
        {
            instances = _instances.Values.ToArray();
        }

        var hasSolo = instances.Any(instance => instance.Solo);
        foreach (var instance in instances)
        {
            var audible = !instance.Mute &&
                (!hasSolo || instance.Solo);
            if (instance.Runtime.Audible != audible)
            {
                instance.Runtime.Audible = audible;
                _dspChanges.MarkChanged(instance.InstanceId);
            }
        }
    }

    private InstanceEntry GetOrCreate(
        InstanceId instanceId,
        DspRuntimeState runtime)
    {
        ArgumentNullException.ThrowIfNull(runtime);

        lock (_lock)
        {
            if (_instances.TryGetValue(instanceId, out var entry))
            {
                if (!ReferenceEquals(entry.Runtime, runtime))
                {
                    throw new InvalidOperationException(
                        $"Audibility runtime already exists for {instanceId}.");
                }

                return entry;
            }

            entry = new InstanceEntry(instanceId, runtime);
            _instances.Add(instanceId, entry);
            return entry;
        }
    }

    private void Attach(
        InstanceEntry entry,
        bool mute,
        bool value)
    {
        lock (_lock)
        {
            entry.ObserverCount++;
            SetValue(entry, mute, value);
        }

        Refresh();
    }

    private void Change(
        InstanceEntry entry,
        bool mute,
        bool value)
    {
        lock (_lock)
        {
            SetValue(entry, mute, value);
        }

        Refresh();
    }

    private void Detach(InstanceEntry entry)
    {
        lock (_lock)
        {
            entry.ObserverCount--;
            if (entry.ObserverCount == 0)
            {
                _instances.Remove(entry.InstanceId);
            }
        }

        Refresh();
    }

    private static void SetValue(
        InstanceEntry entry,
        bool mute,
        bool value)
    {
        if (mute)
        {
            entry.Mute = value;
        }
        else
        {
            entry.Solo = value;
        }
    }

    private sealed class ValueObserver : IStateValueObserver<bool>
    {
        private readonly AudibilityObserver _owner;
        private readonly InstanceId _instanceId;
        private readonly DspRuntimeState _runtime;
        private readonly bool _mute;
        private InstanceEntry? _entry;

        public ValueObserver(
            AudibilityObserver owner,
            InstanceId instanceId,
            DspRuntimeState runtime,
            bool mute)
        {
            _owner = owner;
            _instanceId = instanceId;
            _runtime = runtime;
            _mute = mute;
        }

        public void Attach(StateValue<bool> value)
        {
            _entry = _owner.GetOrCreate(_instanceId, _runtime);
            _owner.Attach(_entry, _mute, value.Value);
        }

        public void ValueChanged(
            StateValue<bool> value,
            bool previousValue,
            bool currentValue)
        {
            _owner.Change(
                _entry ?? throw new InvalidOperationException(
                    "Observer is not attached."),
                _mute,
                currentValue);
        }

        public void Detach(StateValue<bool> value)
        {
            if (_entry is not null)
            {
                _owner.Detach(_entry);
                _entry = null;
            }
        }
    }

    private sealed class InstanceEntry
    {
        public InstanceEntry(
            InstanceId instanceId,
            DspRuntimeState runtime)
        {
            InstanceId = instanceId;
            Runtime = runtime;
        }

        public InstanceId InstanceId { get; }

        public DspRuntimeState Runtime { get; }

        public bool Mute { get; set; }

        public bool Solo { get; set; }

        public int ObserverCount { get; set; }
    }
}
