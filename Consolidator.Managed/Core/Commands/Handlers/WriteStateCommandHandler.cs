using Consolidator.Managed.Core.Commands.Abstractions;
using Consolidator.Managed.Core.Commands.Definitions;
using Consolidator.Managed.State;
using Consolidator.Managed.State.Tree;

namespace Consolidator.Managed.Core.Commands.Handlers;

public sealed class WriteStateCommandHandler
    : CommandHandler<WriteStateCommand, StateWriteStatus>
{
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

        var status = StateWriteStatus.NotHandled;
        foreach (var entry in command.Entries)
        {
            var node = context.State.Root.Find(entry.Path);
            if (node is null || node.IsContainer)
            {
                return ValueTask.FromResult(StateWriteStatus.NotHandled);
            }

            var writer = new WriteValueVisitor(entry.Value, entry.ValueType);
            node.Accept(writer);
            if (writer.Status is StateWriteStatus.NotHandled or StateWriteStatus.Rejected)
            {
                return ValueTask.FromResult(writer.Status);
            }

            status = writer.Status;
        }

        return ValueTask.FromResult(status);
    }

    private sealed class WriteValueVisitor : IStateNodeVisitor
    {
        private readonly object? _value;
        private readonly Type _valueType;

        public WriteValueVisitor(
            object? value,
            Type valueType)
        {
            _value = value;
            _valueType = valueType;
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

            Status = node.Write((TValue)_value!);
        }
    }
}




