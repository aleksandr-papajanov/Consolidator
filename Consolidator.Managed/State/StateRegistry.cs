using Consolidator.Managed.State.History;
using Consolidator.Managed.State.Observers;
using Consolidator.Managed.State.Tree;

namespace Consolidator.Managed.State;

public sealed class StateRegistry<TRootId>
    where TRootId : notnull
{
    private readonly Dictionary<TRootId, StateNode> _roots = new();
    private readonly StateHistory _history;
    private readonly object _lock = new();

    public StateRegistry(StateHistory history)
    {
        ArgumentNullException.ThrowIfNull(history);
        _history = history;
    }

    public StateValue<TValue> CreateValue<TValue>(
        TRootId rootId,
        StatePath path,
        TValue initialValue,
        params IStateValueObserver<TValue>[] observers)
    {
        return CreateValue(rootId, path, initialValue, true, observers);
    }

    public StateValue<TValue> CreateValueWithoutHistory<TValue>(
        TRootId rootId,
        StatePath path,
        TValue initialValue,
        params IStateValueObserver<TValue>[] observers)
    {
        return CreateValue(rootId, path, initialValue, false, observers);
    }

    private StateValue<TValue> CreateValue<TValue>(
        TRootId rootId,
        StatePath path,
        TValue initialValue,
        bool registerInHistory,
        IReadOnlyList<IStateValueObserver<TValue>> observers)
    {
        ArgumentNullException.ThrowIfNull(path);
        ArgumentNullException.ThrowIfNull(observers);

        var parent = GetOrCreateParent(rootId, path);
        EnsureValueSlotAvailable(parent, path);
        var value = new StateValue<TValue>(
            initialValue,
            observers,
            _history.Unregister);
        if (registerInHistory)
        {
            _history.Register(value);
        }
        try
        {
            parent.Add(new StateNode<TValue>(path.Nodes[^1], value));
        }
        catch
        {
            value.Dispose();
            throw;
        }

        return value;
    }

    public void CreateTransient<TValue>(
        TRootId rootId,
        StatePath path,
        Func<TValue> read,
        Action<TValue> write)
    {
        ArgumentNullException.ThrowIfNull(path);
        var parent = GetOrCreateParent(rootId, path);
        EnsureValueSlotAvailable(parent, path);
        parent.Add(new StateNode<TValue>(path.Nodes[^1], read, write));
    }

    public StateNode CreateRoot(TRootId rootId)
    {
        ArgumentNullException.ThrowIfNull(rootId);
        lock (_lock)
        {
            var root = new StateContainerNode(new NodeId(0));
            _roots.Add(rootId, root);
            return root;
        }
    }

    public StateNode GetRoot(TRootId rootId)
    {
        ArgumentNullException.ThrowIfNull(rootId);
        lock (_lock)
        {
            return _roots.TryGetValue(rootId, out var root)
                ? root
                : throw new InvalidOperationException(
                    $"State root was not found: {rootId}.");
        }
    }

    public void RemoveRoot(TRootId rootId)
    {
        ArgumentNullException.ThrowIfNull(rootId);
        StateNode? root;
        lock (_lock)
        {
            _roots.Remove(rootId, out root);
        }

        root?.DisposeValues();
    }

    private StateNode GetOrCreateParent(
        TRootId rootId,
        StatePath path)
    {
        if (path.Depth == 0)
        {
            throw new ArgumentException(
                "A state value requires a non-empty path.",
                nameof(path));
        }

        var parent = GetRoot(rootId);
        for (var index = 0; index < path.Depth - 1; index++)
        {
            var nodeId = path.Nodes[index];
            if (!parent.TryGet(nodeId, out var child) || child is null)
            {
                child = new StateContainerNode(nodeId);
                parent.Add(child);
            }

            if (!child.IsContainer)
            {
                throw new InvalidOperationException(
                    $"State path contains a value node: {path}.");
            }

            parent = child;
        }

        return parent;
    }

    private static void EnsureValueSlotAvailable(
        StateNode parent,
        StatePath path)
    {
        if (parent.TryGet(path.Nodes[^1], out _))
        {
            throw new InvalidOperationException(
                $"State path already exists: {path}.");
        }
    }
}
