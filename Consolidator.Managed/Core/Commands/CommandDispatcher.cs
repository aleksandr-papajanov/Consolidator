using Consolidator.Managed.Core.Commands.Abstractions;

namespace Consolidator.Managed.Core.Commands;

public interface ICommandDispatcher
{
    ValueTask<TResult> DispatchAsync<TResult>(
        IInstanceCommand<TResult> command,
        InstanceCommandContext context,
        CancellationToken cancellationToken);
}

public sealed class CommandDispatcher : ICommandDispatcher
{
    private readonly IReadOnlyDictionary<Type, ICommandHandler> _handlers;

    public CommandDispatcher(IEnumerable<ICommandHandler> handlers)
    {
        ArgumentNullException.ThrowIfNull(handlers);

        _handlers = handlers.ToDictionary(
            handler => handler.CommandType,
            handler => handler);
    }

    public async ValueTask<TResult> DispatchAsync<TResult>(
        IInstanceCommand<TResult> command,
        InstanceCommandContext context,
        CancellationToken cancellationToken)
    {
        ArgumentNullException.ThrowIfNull(command);
        ArgumentNullException.ThrowIfNull(context);

        if (!_handlers.TryGetValue(command.GetType(), out var handler))
        {
            throw new InvalidOperationException(
                $"No command handler is registered for {command.GetType()}.");
        }

        var result = await handler.HandleAsync(
            command,
            context,
            cancellationToken);

        if (result is not TResult typedResult)
        {
            if (result is null && default(TResult) is null)
            {
                return default!;
            }

            throw new InvalidOperationException(
                $"Command handler returned an invalid result for {command.GetType()}.");
        }

        return typedResult;
    }
}




