using Consolidator.Managed.Analyzer;
using Consolidator.Managed.Core.Commands;
using Consolidator.Managed.Core.Commands.Abstractions;
using Consolidator.Managed.Core.Commands.Definitions;
using Consolidator.Managed.Core.Commands.Execution;
using Consolidator.Managed.Core.Commands.Handlers;
using Consolidator.Managed.Core.Commands.Results;
using Consolidator.Managed.Core.Dsp;
using Consolidator.Managed.Core.Services;
using Consolidator.Managed.Core.Services.Abstractions;
using Consolidator.Managed.Core.Services.Instances;
using Consolidator.Managed.Core.State;
using Consolidator.Managed.Core.State.Models;
using Consolidator.Managed.Core.State.Observers;
using Consolidator.Managed.Core.Topology;
using Consolidator.Managed.Native;
using Consolidator.Managed.Protocol;
using Consolidator.Managed.Protocol.Decoding;
using Consolidator.Managed.Protocol.Dispatch;
using Consolidator.Managed.Protocol.Encoding;
using Consolidator.Managed.Protocol.Notifications;
using Consolidator.Managed.Protocol.Transport;
using Consolidator.Managed.Routing.Commands;
using Consolidator.Managed.Routing.Notifications;
using Consolidator.Managed.Services.ServiceExtensions;
using Consolidator.Managed.State;
using Consolidator.Managed.State.History;
using Microsoft.Extensions.DependencyInjection;

namespace Consolidator.Managed.Services;

public static class ManagedServices
{
    private static readonly object ProviderLock = new();
    private static ServiceProvider? _provider;

    public static ServiceProvider Provider
    {
        get
        {
            var provider = Volatile.Read(ref _provider);
            if (provider is not null)
            {
                return provider;
            }

            lock (ProviderLock)
            {
                return _provider ??= CreateProvider();
            }
        }
    }

    internal static void Dispose()
    {
        ServiceProvider? provider;
        lock (ProviderLock)
        {
            provider = _provider;
            _provider = null;
        }

        provider?.Dispose();
    }

    internal static ServiceProvider CreateProvider()
    {
        var services = new ServiceCollection();

        services.AddSingleton(NativeLogSink.Shared);
        services.AddSingleton<NativeOutputService>();
        services.AddSingleton<IProtocolTransport>(serviceProvider =>
            serviceProvider.GetRequiredService<NativeOutputService>());
        services.AddSingleton<IProtocolOutputRegistry>(serviceProvider =>
            serviceProvider.GetRequiredService<NativeOutputService>());
        services.AddSingleton<IOperationGate, OperationGate>();
        services.AddSingleton<IManagedLogger>(serviceProvider =>
            new NativeLogService(
                serviceProvider.GetRequiredService<NativeLogSink>()));
        services.AddSingleton<StateHistory>();
        services.AddSingleton<DspStateChangeTracker>();
        services.AddSingleton<TopologyIndex>();
        services.AddSingleton<StateChangeRouter>();
        services.AddSingleton<StateChangePublisher>();
        services.AddSingleton<HistoryStatePublisher>();
        services.AddSingleton<RegistryChangePublisher>();
        services.AddSingleton<IStateChangeSink>(serviceProvider =>
            serviceProvider.GetRequiredService<StateChangePublisher>());
        services.AddSingleton<StatePeerObserver>();
        services.AddSingleton<AudibilityObserver>();
        services.AddSingleton<StateTopologyObserver>();
        services.AddSingleton<AnalyzerRegistry>();
        services.AddSingleton<StateRegistry<InstanceId>>();
        services.AddSingleton<StateValueMetadataRegistry>();
        services.AddSingleton<TargetStateProjector>();
        services.AddSingleton(serviceProvider =>
            new StateValueFactory(
                serviceProvider.GetRequiredService<StateRegistry<InstanceId>>(),
                serviceProvider.GetRequiredService<StatePeerObserver>(),
                serviceProvider.GetRequiredService<StateValueMetadataRegistry>(),
                serviceProvider.GetRequiredService<IStateChangeSink>(),
                serviceProvider.GetRequiredService<AnalyzerRegistry>(),
                serviceProvider.GetRequiredService<DspStateChangeTracker>()));
        services.AddSingleton(serviceProvider =>
            new InstanceRegistry(
                serviceProvider.GetRequiredService<IManagedLogger>(),
                serviceProvider.GetRequiredService<StateRegistry<InstanceId>>(),
                serviceProvider.GetRequiredService<StateValueFactory>(),
                serviceProvider.GetRequiredService<StateTopologyObserver>(),
                serviceProvider.GetRequiredService<AudibilityObserver>(),
                serviceProvider.GetRequiredService<AnalyzerRegistry>(),
                serviceProvider.GetRequiredService<DspStateChangeTracker>(),
                serviceProvider.GetRequiredService<IOperationGate>(),
                serviceProvider.GetRequiredService<RegistryChangePublisher>(),
                serviceProvider.GetRequiredService<FftAnalyzer>()));
        services.AddSingleton<ICommandHandler, ReadStateCommandHandler>();
        services.AddSingleton<ICommandHandler, WriteStateCommandHandler>();
        services.AddSingleton<ICommandHandler, ResetStateCommandHandler>();
        services.AddSingleton<ICommandHandler, BeginHistoryCommandHandler>();
        services.AddSingleton<ICommandHandler, EndHistoryCommandHandler>();
        services.AddSingleton<ICommandHandler, JumpToHistoryCommandHandler>();
        services.AddSingleton<ICommandHandler, ReadRegistryCommandHandler>();
        services.AddSingleton<ICommandHandler, InitializeUiCommandHandler>();
        services.AddSingleton<ICommandHandler, ObserveTargetCommandHandler>();
        services.AddSingleton<ICommandHandler, SetInstanceActiveCommandHandler>();
        services.AddSingleton<ICommandDispatcher, CommandDispatcher>();
        services.AddSingleton<IStatePathDecoder, StatePathDecoder>();
        services.AddSingleton<IInputCodec, ReadInputCodec>();
        services.AddSingleton<IInputCodec, WriteInputCodec>();
        services.AddSingleton<IInputCodec, ResetInputCodec>();
        services.AddSingleton<IInputCodec>(new TransactionInputCodec(true));
        services.AddSingleton<IInputCodec>(new TransactionInputCodec(false));
        services.AddSingleton<IInputCodec, HistoryInputCodec>();
        services.AddSingleton<IInputCodec, RegistryInputCodec>();
        services.AddSingleton<IInputCodec, InitializeInputCodec>();
        services.AddSingleton<IInputCodec, ObserveTargetInputCodec>();
        services.AddSingleton<IInputCodec, SetInstanceActiveInputCodec>();
        services.AddSingleton<CommandResponseEncoder>();
        services.AddCommandEndpoint<ReadStateCommand, object?>("read", "state_done");
        services.AddCommandEndpoint<WriteStateCommand, StateWriteStatus>("write", "action_done");
        services.AddCommandEndpoint<ResetStateCommand, CommandAcknowledgement>("reset", "action_done");
        services.AddCommandEndpoint<BeginHistoryCommand, CommandAcknowledgement>("begin_history", "action_done");
        services.AddCommandEndpoint<EndHistoryCommand, CommandAcknowledgement>("end_history", "action_done");
        services.AddCommandEndpoint<JumpToHistoryCommand, CommandAcknowledgement>("jump_history", "action_done");
        services.AddCommandEndpoint<ReadRegistryCommand, RegistrySnapshotResult>("registry", "registry_done");
        services.AddCommandEndpoint<InitializeUiCommand, UiInitializationResult>("initialize", "initialized");
        services.AddCommandEndpoint<ObserveTargetCommand, TargetStateSnapshotResult>("observe_target", "target_state_done");
        services.AddCommandEndpoint<SetInstanceActiveCommand, CommandAcknowledgement>("set_instance_active", "action_done");
        services.AddSingleton<CommandEndpointRegistry>();
        services.AddSingleton(serviceProvider =>
            new CommandExecutor(
                serviceProvider.GetRequiredService<InstanceRegistry>(),
                serviceProvider.GetRequiredService<ICommandDispatcher>(),
                serviceProvider.GetRequiredService<AnalyzerRegistry>(),
                serviceProvider.GetRequiredService<DspStateChangeTracker>()));
        services.AddSingleton(serviceProvider =>
            new InstanceCommandRouter(
                serviceProvider.GetRequiredService<InstanceRegistry>(),
                serviceProvider.GetRequiredService<TopologyIndex>(),
                serviceProvider.GetRequiredService<CommandExecutor>(),
                serviceProvider.GetRequiredService<IOperationGate>(),
                serviceProvider.GetRequiredService<StatePeerObserver>()));
        services.AddSingleton<CommandDecoder>();
        services.AddSingleton<ProtocolService>(serviceProvider =>
            new ProtocolService(
                serviceProvider.GetRequiredService<CommandDecoder>(),
                serviceProvider.GetRequiredService<CommandEndpointRegistry>(),
                serviceProvider.GetRequiredService<IProtocolTransport>()));
        services.AddSingleton<IInstanceLifecycleService>(serviceProvider =>
            serviceProvider.GetRequiredService<InstanceRegistry>());
        services.AddSingleton<IInstancePreparationService, InstancePreparationService>();
        services.AddSingleton<FftAnalyzer>(serviceProvider =>
            new FftAnalyzer(
                serviceProvider.GetRequiredService<StateTopologyObserver>(),
                serviceProvider.GetRequiredService<IProtocolTransport>(),
                serviceProvider.GetRequiredService<AnalyzerRegistry>()));
        services.AddSingleton<IInstanceAudioInputService>(serviceProvider =>
            serviceProvider.GetRequiredService<FftAnalyzer>());
        services.AddSingleton<IInstancePreparationHandler>(serviceProvider =>
            serviceProvider.GetRequiredService<FftAnalyzer>());
        services.AddSingleton<IHistoryNavigation>(serviceProvider =>
            new HistoryNavigation(
                serviceProvider.GetRequiredService<StateHistory>(),
                serviceProvider.GetRequiredService<InstanceRegistry>(),
                serviceProvider.GetRequiredService<AnalyzerRegistry>(),
                serviceProvider.GetRequiredService<DspStateChangeTracker>(),
                serviceProvider.GetRequiredService<HistoryStatePublisher>()));

        return services.BuildServiceProvider(
            new ServiceProviderOptions
            {
                ValidateOnBuild = true,
                ValidateScopes = true
            });
    }
}
