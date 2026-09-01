using Consolidator.Managed.Core.Commands.Abstractions;
using Consolidator.Managed.Core.Commands.Definitions;
using Consolidator.Managed.Core.State;
using Consolidator.Managed.State;
using Consolidator.Managed.State.History;
using Consolidator.Managed.State.Tree;

namespace Consolidator.Managed.Core.Commands.Handlers;

public sealed class WriteStateCommandHandler
    : CommandHandler<WriteStateCommand, StateWriteStatus>
{
    private readonly StateHistory _history;
    private readonly IReadOnlyList<IStateWritePolicy> _policies;

    public WriteStateCommandHandler(
        StateHistory history,
        IEnumerable<IStateWritePolicy> policies)
    {
        ArgumentNullException.ThrowIfNull(history);
        ArgumentNullException.ThrowIfNull(policies);
        _history = history;
        _policies = policies.ToArray();
    }

    public override ValueTask<StateWriteStatus> HandleAsync(
        WriteStateCommand command,
        InstanceCommandContext context,
        CancellationToken cancellationToken)
    {
        cancellationToken.ThrowIfCancellationRequested();

        if (command.Entries.Count == 0)
        {
            return ValueTask.FromResult(StateWriteStatus.NotHandled);
        }

        var entries = command.Entries
            .Select(entry => entry with { Path = context.ResolvePath(entry.Path) })
            .ToArray();
        var resolvedCommand = command with { Entries = entries };
        if (!AreWritesAllowed(resolvedCommand, context))
        {
            return ValueTask.FromResult(StateWriteStatus.Rejected);
        }

        var writes = new List<(StateNode Node, StateWriteEntry Entry)>(entries.Length);
        var paths = new HashSet<StatePath>();
        foreach (var entry in entries)
        {
            var node = context.State.Root.Find(entry.Path);
            if (node is null || node.IsContainer || !paths.Add(entry.Path))
            {
                return ValueTask.FromResult(StateWriteStatus.NotHandled);
            }

            var validator = new WriteValueVisitor(
                entry.Value,
                entry.ValueType,
                entry.Mode,
                null);
            node.Accept(validator);
            if (validator.Status is StateWriteStatus.NotHandled or StateWriteStatus.Rejected)
            {
                return ValueTask.FromResult(validator.Status);
            }

            writes.Add((node, entry));
        }

        var status = StateWriteStatus.Unchanged;
        using var transaction = _history.BeginTransaction();
        foreach (var write in writes)
        {
            var writer = new WriteValueVisitor(
                write.Entry.Value,
                write.Entry.ValueType,
                write.Entry.Mode,
                transaction);
            write.Node.Accept(writer);
            if (writer.Status is StateWriteStatus.NotHandled or StateWriteStatus.Rejected)
            {
                return ValueTask.FromResult(writer.Status);
            }
            if (writer.Status is StateWriteStatus.Applied)
            {
                status = StateWriteStatus.Applied;
            }
        }
        transaction.Commit();
        return ValueTask.FromResult(status);
    }

    private bool AreWritesAllowed(
        WriteStateCommand command,
        InstanceCommandContext context)
    {
        foreach (var policy in _policies)
        {
            if (command.Entries.Any(entry => policy.Applies(entry.Path)) &&
                !policy.IsAllowed(command, context))
            {
                return false;
            }
        }

        return true;
    }

    private sealed class WriteValueVisitor : IStateNodeVisitor
    {
        private readonly object? _value;
        private readonly Type _valueType;
        private readonly StateValueEditMode _mode;
        private readonly StateHistoryTransaction? _transaction;

        public WriteValueVisitor(
            object? value,
            Type valueType,
            StateValueEditMode mode,
            StateHistoryTransaction? transaction)
        {
            _value = value;
            _valueType = valueType;
            _mode = mode;
            _transaction = transaction;
        }

        public StateWriteStatus Status { get; private set; } =
            StateWriteStatus.NotHandled;

        public void VisitContainer(StateContainerNode node)
        {
        }

        public void Visit<TValue>(StateNode<TValue> node)
        {
            if (_valueType != typeof(TValue))
            {
                Status = StateWriteStatus.Rejected;
                return;
            }

            Status = _transaction is null
                ? EqualityComparer<TValue>.Default.Equals(node.Value, (TValue)_value!)
                    ? StateWriteStatus.Unchanged
                    : StateWriteStatus.Applied
                : node.PrepareWrite((TValue)_value!, _mode, _transaction);
        }
    }
}




