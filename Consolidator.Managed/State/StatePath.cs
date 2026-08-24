using Consolidator.Managed.State.Tree;

namespace Consolidator.Managed.State;

public sealed class StatePath : IEquatable<StatePath>
{
    private readonly NodeId[] _nodes;

    public StatePath(IEnumerable<NodeId> nodes)
    {
        ArgumentNullException.ThrowIfNull(nodes);
        _nodes = nodes.ToArray();
    }

    public IReadOnlyList<NodeId> Nodes => _nodes;

    public int Depth => _nodes.Length;

    public static StatePath Empty { get; } = new(Array.Empty<NodeId>());

    public StatePath Append(NodeId nodeId) => new(_nodes.Append(nodeId));

    public bool Matches(StatePath candidate)
    {
        ArgumentNullException.ThrowIfNull(candidate);
        return Depth <= candidate.Depth &&
            _nodes.SequenceEqual(candidate._nodes.Take(Depth));
    }

    public bool Equals(StatePath? other) =>
        other is not null && _nodes.SequenceEqual(other._nodes);

    public override bool Equals(object? obj) => Equals(obj as StatePath);

    public override int GetHashCode()
    {
        var hash = new HashCode();
        foreach (var node in _nodes)
        {
            hash.Add(node);
        }

        return hash.ToHashCode();
    }
}




