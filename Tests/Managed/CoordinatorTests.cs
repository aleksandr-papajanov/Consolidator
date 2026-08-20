using System;
using System.Runtime.InteropServices;
using System.Threading;
using System.Threading.Tasks;
using Consolidator.Managed.Core;
using Consolidator.Managed.Core.Abstractions;
using Consolidator.Managed.Core.Dsp;
using Consolidator.Managed.Core.Instances;
using Consolidator.Managed.Core.State;
using Consolidator.Managed.Core.State.Bindings;
using Consolidator.Managed.Core.Topology;
using Consolidator.Managed.Native;
using Consolidator.Managed.Protocol;
using Microsoft.Extensions.DependencyInjection;
using Xunit;

namespace Consolidator.Managed.Tests;

public sealed class CoordinatorTests
{
    [Fact]
    public void ManagedServicesProvidesOneCoordinatorSingleton()
    {
        var first = ManagedServices.Provider.GetRequiredService<Coordinator>();
        var second = ManagedServices.Provider.GetRequiredService<Coordinator>();
        var firstRegistry = ManagedServices.Provider.GetRequiredService<InstanceRegistry>();
        var secondRegistry = ManagedServices.Provider.GetRequiredService<InstanceRegistry>();
        var firstTopologyStore = ManagedServices.Provider.GetRequiredService<TopologyStore>();
        var secondTopologyStore = ManagedServices.Provider.GetRequiredService<TopologyStore>();

        Assert.Same(first, second);
        Assert.Same(firstRegistry, secondRegistry);
        Assert.Same(firstTopologyStore, secondTopologyStore);
    }

    [Fact]
    public unsafe void CoordinatorIssuesDistinctIdsForMultipleInstances()
    {
        var history = new StateHistory();
        var coordinator = new Coordinator(
            new TestLogger(),
            history,
            new DspStateCompiler(),
            new InstanceStateBuilder(),
            new InstanceRegistry(),
            new TopologyStore(history));
        var firstExchange = AllocateExchange();
        var secondExchange = AllocateExchange();
        var firstPublisher = new NativeDspStatePublisher(firstExchange);
        var secondPublisher = new NativeDspStatePublisher(secondExchange);

        var first = coordinator.RegisterInstance(new TestOutput(), firstPublisher);
        var second = coordinator.RegisterInstance(new TestOutput(), secondPublisher);

        try
        {
            Assert.NotEqual(0UL, first.Id);
            Assert.NotEqual(first.Id, second.Id);
        }
        finally
        {
            coordinator.UnregisterInstance(first.Id);
            coordinator.UnregisterInstance(second.Id);
            NativeMemory.Free(firstExchange);
            NativeMemory.Free(secondExchange);
        }
    }

    [Fact]
    public void CoordinatorUndoRestoresAllInstancesAndPublishesEachSnapshotOnce()
    {
        var history = new StateHistory();
        var coordinator = new Coordinator(
            new TestLogger(),
            history,
            new DspStateCompiler(),
            new InstanceStateBuilder(),
            new InstanceRegistry(),
            new TopologyStore(history));
        var firstPublisher = new RecordingDspStatePublisher();
        var secondPublisher = new RecordingDspStatePublisher();
        var first = coordinator.RegisterInstance(new TestOutput(), firstPublisher);
        var second = coordinator.RegisterInstance(new TestOutput(), secondPublisher);

        try
        {
            second.StateStore.Find<float>(StateIds.Gain)!.Value = 10.0F;

            coordinator.AdvanceHistoryPoint();
            first.StateStore.Find<float>(StateIds.Gain)!.Value = 2.0F;
            second.StateStore.Find<float>(StateIds.Gain)!.Value = 20.0F;

            coordinator.AdvanceHistoryPoint();
            first.StateStore.Find<float>(StateIds.Gain)!.Value = 3.0F;
            second.StateStore.Find<float>(StateIds.Gain)!.Value = 30.0F;

            Assert.True(coordinator.UndoHistory());

            Assert.Equal(2.0F, first.StateStore.Find<float>(StateIds.Gain)!.Value);
            Assert.Equal(20.0F, second.StateStore.Find<float>(StateIds.Gain)!.Value);
            Assert.Equal(2.0F, firstPublisher.LastSnapshot.Gain);
            Assert.Equal(20.0F, secondPublisher.LastSnapshot.Gain);
            Assert.Equal(2, firstPublisher.PublishCount);
            Assert.Equal(2, secondPublisher.PublishCount);
        }
        finally
        {
            coordinator.UnregisterInstance(first.Id);
            coordinator.UnregisterInstance(second.Id);
        }
    }

    [Fact]
    public void CoordinatorStoresPerInstanceTopologyInSharedHistory()
    {
        var history = new StateHistory();
        var coordinator = new Coordinator(
            new TestLogger(),
            history,
            new DspStateCompiler(),
            new InstanceStateBuilder(),
            new InstanceRegistry(),
            new TopologyStore(history));
        var first = coordinator.RegisterInstance(
            new TestOutput(),
            new RecordingDspStatePublisher());
        var second = coordinator.RegisterInstance(
            new TestOutput(),
            new RecordingDspStatePublisher());

        try
        {
            coordinator.AdvanceHistoryPoint();
            Assert.True(coordinator.SetBankGroup(first.Id, 0, new GroupId(12)));
            Assert.True(coordinator.SetBankGroup(second.Id, 5, new GroupId(12)));
            Assert.True(coordinator.SetBankGroup(second.Id, 6, new GroupId(24)));
            Assert.True(coordinator.SetFocusedBank(
                first.Id,
                new BankAddress(second.Id, 6)));

            Assert.Equal(new GroupId(12), coordinator.GetTopology(first.Id)!.GetGroupId(0));
            Assert.Equal(new GroupId(12), coordinator.GetTopology(second.Id)!.GetGroupId(5));
            Assert.Equal(new GroupId(24), coordinator.GetTopology(second.Id)!.GetGroupId(6));
            Assert.Equal(
                new BankAddress(second.Id, 6),
                coordinator.GetTopology(first.Id)!.FocusedBank);
            var groupMembers = coordinator.GetGroupMembers(new BankAddress(first.Id, 0));
            Assert.Equal(2, groupMembers.Count);
            Assert.Contains(new BankAddress(first.Id, 0), groupMembers);
            Assert.Contains(new BankAddress(second.Id, 5), groupMembers);
            Assert.Same(
                second,
                coordinator.Resolve(new BankAddress(second.Id, 5)));

            var groupInstances = coordinator.GetGroupInstanceIds(new GroupId(12));
            Assert.Equal(2, groupInstances.Count);
            Assert.Contains(first.Id, groupInstances);
            Assert.Contains(second.Id, groupInstances);
            Assert.Equal(2, coordinator.GetGroupInstances(new GroupId(12)).Count);
            Assert.Contains(
                new BankAddress(second.Id, 6),
                coordinator.GetConnectedGroupBanks(
                    new[] { new BankAddress(first.Id, 0) }));

            Assert.True(coordinator.UndoHistory());

            Assert.Equal(TopologyState.Empty, coordinator.GetTopology(first.Id));
            Assert.Equal(TopologyState.Empty, coordinator.GetTopology(second.Id));
        }
        finally
        {
            coordinator.UnregisterInstance(first.Id);
            coordinator.UnregisterInstance(second.Id);
        }
    }

    [Fact]
    public unsafe void DspPublisherPublishesToTheOnlyWritableTripleBufferSlot()
    {
        var exchange = AllocateExchange();

        try
        {
            var publisher = new NativeDspStatePublisher(exchange);

            publisher.Publish(new DspSnapshot { Gain = 0.5F });

            Assert.Equal(1U, exchange->PublishedIndex);
            Assert.Equal(0.5F, exchange->Snapshot1.Gain);

            publisher.Publish(new DspSnapshot { Gain = 0.75F });

            Assert.Equal(2U, exchange->PublishedIndex);
            Assert.Equal(0.75F, exchange->Snapshot2.Gain);

            exchange->ConsumerIndex = exchange->PublishedIndex;

            publisher.Publish(new DspSnapshot { Gain = 0.25F });

            Assert.Equal(0U, exchange->PublishedIndex);
            Assert.Equal(0.25F, exchange->Snapshot0.Gain);
        }
        finally
        {
            NativeMemory.Free(exchange);
        }
    }

    [Fact]
    public unsafe void DspPublisherLatestSnapshotWinsWhenConsumerLags()
    {
        var exchange = AllocateExchange();

        try
        {
            var publisher = new NativeDspStatePublisher(exchange);

            publisher.Publish(new DspSnapshot { Gain = 0.2F });
            publisher.Publish(new DspSnapshot { Gain = 0.4F });
            publisher.Publish(new DspSnapshot { Gain = 0.8F });

            var publishedIndex = exchange->PublishedIndex;
            exchange->ConsumerIndex = publishedIndex;

            Assert.Equal(1U, publishedIndex);
            Assert.Equal(0.8F, exchange->Snapshot1.Gain);
        }
        finally
        {
            NativeMemory.Free(exchange);
        }
    }

    [Fact]
    public void DspDefaultsCreateUnityGainState()
    {
        var state = DspDefaults.CreateState();

        Assert.Equal(1.0F, state.Gain);
    }

    [Fact]
    public void DspStateCompilerCreatesRuntimeSnapshotFromDspState()
    {
        var state = new DspState
        {
            Gain = 0.5F
        };

        var snapshot = new DspStateCompiler().Compile(state);

        Assert.Equal(0.5F, snapshot.Gain);
    }

    [Fact]
    public unsafe void PrepareDoesNotResetPublishedDspSnapshot()
    {
        var exchange = AllocateExchange();
        var publisher = new NativeDspStatePublisher(exchange);
        var instance = new ConsolidatorInstance(
            1,
            new TestOutput(),
            publisher,
            new StateHistory(),
            new DspStateCompiler(),
            DspDefaults.CreateState(),
            new InstanceStateBuilder());

        try
        {
            publisher.Publish(new DspSnapshot { Gain = 0.5F });
            instance.Prepare(48000, 0);

            Assert.Equal(2U, exchange->PublishedIndex);
            Assert.Equal(0.5F, exchange->Snapshot2.Gain);
        }
        finally
        {
            instance.Stop();
            NativeMemory.Free(exchange);
        }
    }

    [Fact]
    public async Task UnregisterWaitsForActiveOutput()
    {
        using var outputEntered = new ManualResetEventSlim();
        using var releaseOutput = new ManualResetEventSlim();
        var output = new BlockingOutput(outputEntered, releaseOutput);
        var exchangeAddress = AllocateExchangeAddress();
        var publisher = CreatePublisher(exchangeAddress);
        var instance = new ConsolidatorInstance(
            1,
            output,
            publisher,
            new StateHistory(),
            new DspStateCompiler(),
            DspDefaults.CreateState(),
            new InstanceStateBuilder());

        try
        {
            var sendTask = Task.Run(() => instance.TrySend(
                "ready",
                Array.Empty<Atom>()));

            Assert.True(outputEntered.Wait(TimeSpan.FromSeconds(5)));

            var unregisterTask = Task.Run(instance.Stop);

            var completedTask = await Task.WhenAny(
                unregisterTask,
                Task.Delay(TimeSpan.FromMilliseconds(100)));

            Assert.NotSame(unregisterTask, completedTask);

            releaseOutput.Set();
            await sendTask;
            await unregisterTask;

            publisher.Publish(new DspSnapshot { Gain = 0.25F });
            Assert.Equal(1U, ReadPublishedIndex(exchangeAddress));
        }
        finally
        {
            FreeExchange(exchangeAddress);
        }
    }

    [Fact]
    public unsafe void UnregisterStopsDspPublisherBeforeExchangeIsFreed()
    {
        var history = new StateHistory();
        var coordinator = new Coordinator(
            new TestLogger(),
            history,
            new DspStateCompiler(),
            new InstanceStateBuilder(),
            new InstanceRegistry(),
            new TopologyStore(history));
        var exchange = AllocateExchange();
        var publisher = new NativeDspStatePublisher(exchange);
        var instance = coordinator.RegisterInstance(
            new TestOutput(),
            publisher);

        try
        {
            publisher.Publish(new DspSnapshot { Gain = 0.5F });
            Assert.Equal(2U, exchange->PublishedIndex);

            coordinator.UnregisterInstance(instance.Id);
            NativeMemory.Free(exchange);
            exchange = null;

            publisher.Publish(new DspSnapshot { Gain = 0.25F });
        }
        finally
        {
            if (exchange != null)
            {
                NativeMemory.Free(exchange);
            }
        }
    }

    [Fact]
    public unsafe void StoppedAudioInputIgnoresLateAudioAfterInstanceStop()
    {
        var exchange = AllocateExchange();
        var publisher = new NativeDspStatePublisher(exchange);
        var instance = new ConsolidatorInstance(
            1,
            new TestOutput(),
            publisher,
            new StateHistory(),
            new DspStateCompiler(),
            DspDefaults.CreateState(),
            new InstanceStateBuilder());
        var audioInput = new NativeAudioInput(instance);

        try
        {
            instance.Stop();
            audioInput.ReceiveAudio(
                null,
                null,
                null,
                null,
                0);
        }
        finally
        {
            NativeMemory.Free(exchange);
        }
    }

    [Fact]
    public async Task StopWaitsForActiveDspPublish()
    {
        using var publishEntered = new ManualResetEventSlim();
        using var releasePublish = new ManualResetEventSlim();
        var publisher = new BlockingDspStatePublisher(
            publishEntered,
            releasePublish);
        var instance = new ConsolidatorInstance(
            1,
            new TestOutput(),
            publisher,
            new StateHistory(),
            new DspStateCompiler(),
            DspDefaults.CreateState(),
            new InstanceStateBuilder());

        var publishTask = Task.Run(() => publisher.Publish(
            new DspSnapshot { Gain = 0.5F }));

        Assert.True(publishEntered.Wait(TimeSpan.FromSeconds(5)));

        var stopTask = Task.Run(instance.Stop);
        var completedTask = await Task.WhenAny(
            stopTask,
            Task.Delay(TimeSpan.FromMilliseconds(100)));

        Assert.NotSame(stopTask, completedTask);

        releasePublish.Set();
        await publishTask;
        await stopTask;
    }

    private static unsafe SharedDspExchange* AllocateExchange()
    {
        var exchange =
            (SharedDspExchange*)NativeMemory.Alloc(
                (nuint)sizeof(SharedDspExchange));
        *exchange = default;
        return exchange;
    }

    private static nint AllocateExchangeAddress()
    {
        unsafe
        {
            return (nint)AllocateExchange();
        }
    }

    private static NativeDspStatePublisher CreatePublisher(
        nint exchangeAddress)
    {
        unsafe
        {
            return new NativeDspStatePublisher(
                (SharedDspExchange*)exchangeAddress);
        }
    }

    private static void FreeExchange(nint exchangeAddress)
    {
        unsafe
        {
            NativeMemory.Free((void*)exchangeAddress);
        }
    }

    private static uint ReadPublishedIndex(nint exchangeAddress)
    {
        unsafe
        {
            return ((SharedDspExchange*)exchangeAddress)->PublishedIndex;
        }
    }

    private sealed class TestOutput : IInstanceOutput
    {
        public void Send(
            string selector,
            ReadOnlySpan<Atom> atoms)
        {
        }
    }

    private sealed class BlockingOutput : IInstanceOutput
    {
        private readonly ManualResetEventSlim _entered;
        private readonly ManualResetEventSlim _release;

        public BlockingOutput(
            ManualResetEventSlim entered,
            ManualResetEventSlim release)
        {
            _entered = entered;
            _release = release;
        }

        public void Send(
            string selector,
            ReadOnlySpan<Atom> atoms)
        {
            _entered.Set();
            _release.Wait();
        }
    }

    private sealed class BlockingDspStatePublisher : IDspStatePublisher
    {
        private readonly object _publishLock = new();
        private readonly ManualResetEventSlim _publishEntered;
        private readonly ManualResetEventSlim _releasePublish;
        private int _publishCount;

        public BlockingDspStatePublisher(
            ManualResetEventSlim publishEntered,
            ManualResetEventSlim releasePublish)
        {
            _publishEntered = publishEntered;
            _releasePublish = releasePublish;
        }

        public void Publish(in DspSnapshot snapshot)
        {
            lock (_publishLock)
            {
                if (Interlocked.Increment(ref _publishCount) == 1)
                {
                    return;
                }

                _publishEntered.Set();
                _releasePublish.Wait();
            }
        }

        public void Stop()
        {
            lock (_publishLock)
            {
            }
        }
    }

    private sealed class RecordingDspStatePublisher : IDspStatePublisher
    {
        public DspSnapshot LastSnapshot { get; private set; }

        public int PublishCount { get; private set; }

        public void Publish(in DspSnapshot snapshot)
        {
            LastSnapshot = snapshot;
            PublishCount++;
        }

        public void Stop()
        {
        }
    }

    private sealed class TestLogger : IConsolidatorLogger
    {
        public void Info(string message)
        {
        }

        public void Warning(string message)
        {
        }

        public void Error(string message)
        {
        }
    }
}