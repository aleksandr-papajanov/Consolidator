namespace Consolidator.Managed.Core.Commands.Abstractions;

public interface ICommandHandler
{
    Type CommandType { get; }

    ValueTask<object?> HandleAsync(
        object command,
        InstanceCommandContext context,
        CancellationToken cancellationToken);
}

public interface ICommandHandler<TCommand, TResult> : ICommandHandler
    where TCommand : IInstanceCommand<TResult>
{
    ValueTask<TResult> HandleAsync(
        TCommand command,
        InstanceCommandContext context,
        CancellationToken cancellationToken);
}

public abstract class CommandHandler<TCommand, TResult> : ICommandHandler<TCommand, TResult>
    where TCommand : IInstanceCommand<TResult>
{
    public Type CommandType => typeof(TCommand);

    public abstract ValueTask<TResult> HandleAsync(
        TCommand command,
        InstanceCommandContext context,
        CancellationToken cancellationToken);

    async ValueTask<object?> ICommandHandler.HandleAsync(
        object command,
        InstanceCommandContext context,
        CancellationToken cancellationToken)
    {
        if (command is not TCommand typedCommand)
        {
            throw new ArgumentException(
                $"Command type does not match handler: {command.GetType()}.",
                nameof(command));
        }

        return await HandleAsync(
            typedCommand,
            context,
            cancellationToken);
    }
}




