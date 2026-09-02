using Consolidator.Managed.Core.Commands.Abstractions;
using Consolidator.Managed.Protocol.Dispatch;
using Consolidator.Managed.Protocol.Encoding;
using Consolidator.Managed.Core.Commands.Routing;
using Microsoft.Extensions.DependencyInjection;

namespace Consolidator.Managed.Composition.ServiceExtensions;

internal static class CommandEndpointServiceCollectionExtensions
{
    public static IServiceCollection AddCommandEndpoint<TCommand, TResult>(
        this IServiceCollection services,
        string selector,
        string responseSelector)
        where TCommand : IInstanceCommand<TResult>
    {
        return services.AddSingleton<ICommandEndpoint>(serviceProvider =>
            new CommandEndpoint<TCommand, TResult>(
                serviceProvider.GetRequiredService<InstanceCommandRouter>(),
                serviceProvider.GetRequiredService<CommandResponseEncoder>(),
                selector,
                responseSelector));
    }
}



