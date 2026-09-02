using Consolidator.Managed.State.History;

namespace Consolidator.Managed.State.Tree;

public abstract class StateNode : IStateNode
{
    private readonly Dictionary<NodeId, StateNode> _children = new();

    protected StateNode(NodeId id)
    {
        Id = id;
    }

    public NodeId Id { get; }

    public abstract bool IsContainer { get; }

    public IReadOnlyDictionary<NodeId, StateNode> Children => _children;

    public StateNode Add(StateNode child)
    {
        ArgumentNullException.ThrowIfNull(child);
        _children.Add(child.Id, child);
        return child;
    }

    public bool TryGet(NodeId id, out StateNode? child) =>
        _children.TryGetValue(id, out child);

    public StateNode? Find(StatePath path)
    {
        ArgumentNullException.ThrowIfNull(path);

        var current = this;
        foreach (var nodeId in path.Nodes)
        {
            if (!current.TryGet(nodeId, out var child) || child is null)
            {
                return null;
            }

            current = child;
        }

        return current;
    }

    internal abstract void Accept(IStateNodeVisitor visitor);

    internal virtual void DisposeValues()
    {
        foreach (var child in _children.Values)
        {
            child.DisposeValues();
        }
    }

    internal virtual int PrepareResetRecursive(
        StateHistoryTransaction transaction)
    {
        ArgumentNullException.ThrowIfNull(transaction);
        return Children.Values.Sum(child =>
            child.PrepareResetRecursive(transaction));
    }

}

internal interface IStateNodeVisitor
{
    void VisitContainer(StateContainerNode node);

    void Visit<TValue>(StateNode<TValue> node);
}

internal sealed class StateContainerNode : StateNode
{
    public StateContainerNode(NodeId id)
        : base(id)
    {
    }

    public override bool IsContainer => true;

    internal override void Accept(IStateNodeVisitor visitor)
    {
        ArgumentNullException.ThrowIfNull(visitor);
        visitor.VisitContainer(this);
    }

}

internal sealed class StateNode<TValue> : StateNode, IStateNode<TValue>
{
    private readonly Func<TValue> _read;
    private readonly Action<TValue> _write;
    private readonly StateValue<TValue>? _historyValue;

    public StateNode(
        NodeId id,
        StateValue<TValue> value)
        : base(id)
    {
        ArgumentNullException.ThrowIfNull(value);
        _historyValue = value;
        _read = () => value.Value;
        _write = newValue => value.Value = newValue;
    }

    public StateNode(
        NodeId id,
        Func<TValue> read,
        Action<TValue> write)
        : base(id)
    {
        ArgumentNullException.ThrowIfNull(read);
        ArgumentNullException.ThrowIfNull(write);
        _read = read;
        _write = write;
    }

    public TValue Value => _read();

    public override bool IsContainer => false;

    internal override void Accept(IStateNodeVisitor visitor)
    {
        ArgumentNullException.ThrowIfNull(visitor);
        visitor.Visit(this);
    }

    public StateWriteStatus Write(TValue value)
    {
        if (EqualityComparer<TValue>.Default.Equals(Value, value))
        {
            return StateWriteStatus.Unchanged;
        }

        _write(value);
        return StateWriteStatus.Applied;
    }

    internal StateWriteStatus PrepareWrite(
        TValue value,
        StateValueEditMode editMode,
        StateHistoryTransaction transaction)
    {
        ArgumentNullException.ThrowIfNull(transaction);
        if (_historyValue is null)
        {
            return StateWriteStatus.Rejected;
        }
        if (EqualityComparer<TValue>.Default.Equals(Value, value))
        {
            return StateWriteStatus.Unchanged;
        }

        _historyValue.PrepareMutation(value, editMode, transaction);
        return StateWriteStatus.Applied;
    }

    internal override int PrepareResetRecursive(
        StateHistoryTransaction transaction)
    {
        ArgumentNullException.ThrowIfNull(transaction);
        return _historyValue?.PrepareReset(transaction) is true ? 1 : 0;
    }

    internal override void DisposeValues()
    {
        _historyValue?.Dispose();
    }

}




