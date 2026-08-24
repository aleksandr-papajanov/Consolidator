namespace Consolidator.Managed.State.Tree;

public interface IStateNode
{
    NodeId Id { get; }

    bool IsContainer { get; }
}

public interface IStateNode<TValue> : IStateNode
{
    TValue Value { get; }

    StateWriteStatus Write(TValue value);
}




