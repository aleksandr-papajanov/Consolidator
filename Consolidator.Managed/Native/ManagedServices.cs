using Consolidator.Managed.Core;
using Consolidator.Managed.Core.Abstractions;
using Consolidator.Managed.Core.Dsp;
using Consolidator.Managed.Core.State;
using Microsoft.Extensions.DependencyInjection;

namespace Consolidator.Managed.Native;

public static class ManagedServices
{
    public static ServiceProvider Provider { get; } = BuildProvider();

    private static ServiceProvider BuildProvider()
    {
        var services = new ServiceCollection();

        services.AddSingleton<NativeLogSink>();
        services.AddSingleton<IConsolidatorLogger>(serviceProvider =>
            new ConsolidatorLogger(
                serviceProvider.GetRequiredService<NativeLogSink>()));
        services.AddSingleton<StateHistory>();
        services.AddSingleton<DspStateCompiler>();
        services.AddSingleton<Coordinator>();
        services.AddSingleton<ConsolidatorCore>(serviceProvider =>
            new ConsolidatorCore(
                serviceProvider.GetRequiredService<Coordinator>()));

        return services.BuildServiceProvider(
            new ServiceProviderOptions
            {
                ValidateOnBuild = true,
                ValidateScopes = true
            });
    }
}
