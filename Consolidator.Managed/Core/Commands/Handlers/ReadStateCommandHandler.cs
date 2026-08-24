using Consolidator.Managed.Core.Commands.Abstractions;
using Consolidator.Managed.Core.Commands.Definitions;
using Consolidator.Managed.State.Tree;

namespace Consolidator.Managed.Core.Commands.Handlers;

public sealed class ReadStateCommandHandler
    : CommandHandler<ReadStateCommand, object?>
{
    public override ValueTask<object?> HandleAsync(
        ReadStateCommand command,
        InstanceCommandContext context,
        CancellationToken cancellationToken)
    {
        cancellationToken.ThrowIfCancellationRequested();

        var node = context.State.Root.Find(command.Path)
            ?? throw new KeyNotFoundException(
                $"State path was not found: {command.Path}.");
        if (node.IsContainer)
        {
            throw new InvalidOperationException(
                $"State path points to a container: {command.Path}.");
        }

        var reader = new ReadValueVisitor();
        node.Accept(reader);
        return ValueTask.FromResult(reader.Value);
    }

    private sealed class ReadValueVisitor : IStateNodeVisitor
    {
        public object? Value { get; private set; }

        public void VisitContainer(StateContainerNode node)
        {
        }

        public void Visit<TValue>(StateNode<TValue> node)
        {
            Value = node.Value;
        }
    }
}




