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

        var node = context.State.Root.Find(command.Path);
        if (node is null || node.IsContainer)
        {
            return ValueTask.FromResult(StateWriteStatus.NotHandled);
        }

        var writer = new WriteValueVisitor(
            command.Value,
            command.ValueType);
        node.Accept(writer);
        return ValueTask.FromResult(writer.Status);
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




