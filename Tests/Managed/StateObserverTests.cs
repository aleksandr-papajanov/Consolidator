using Consolidator.Managed.Core.Dsp;
using Consolidator.Managed.Core.Commands.Abstractions;
using Consolidator.Managed.Core.Commands.Definitions;
using Consolidator.Managed.Core.Commands.Handlers;
using Consolidator.Managed.Core.Services.Abstractions;
using Consolidator.Managed.Core.State;
using Consolidator.Managed.Core.State.Models;
using Consolidator.Managed.Core.State.Observers;
using Consolidator.Managed.Core.Topology;
using Consolidator.Managed.Routing.Notifications;
using Consolidator.Managed.State;
using Consolidator.Managed.State.History;
using Consolidator.Managed.State.Observers;
using Xunit;

namespace Consolidator.Managed.Tests;

public sealed class StateObserverTests
{
    [Fact]
    public void ValueObserversReceiveInitialAndEffectiveChanges()
    {
        var fixture = new StateFixture();
        var instanceId = new InstanceId(1);
        fixture.Storage.CreateRoot(instanceId);
        var projectedValue = -1;
        var value = fixture.Values.CreateValue(
            instanceId,
            new StatePath([StateNodeIds.Instance, StateNodeIds.Label]),
            10,
            StateValueEditMode.CopyValue,
            observers: [new StateProjectionObserver<int>(current => projectedValue = current)]);

        Assert.Equal(10, projectedValue);
        Assert.Empty(fixture.Changes);

        fixture.History.AdvanceHistoryPoint();
        value.Value = 20;
        value.Value = 20;

        Assert.Equal(20, projectedValue);
        Assert.Single(fixture.Changes);

        Assert.True(fixture.History.Undo());
        Assert.Equal(10, projectedValue);
        Assert.Equal(2, fixture.Changes.Count);

        fixture.Storage.RemoveRoot(instanceId);
    }

    [Fact]
    public async Task StateCommandHandlersReadAndWriteTypedValues()
    {
        var fixture = new StateFixture();
        var instance = fixture.CreateInstance(new InstanceId(1));
        var path = new StatePath([
            StateNodeIds.Dsp,
            StateNodeIds.InputGain,
            StateNodeIds.Gain]);
        var context = new InstanceCommandContext(
            instance.State.InstanceId,
            instance.ManagedState);
        var writer = new WriteStateCommandHandler();
        var reader = new ReadStateCommandHandler();

        var status = await writer.HandleAsync(
            WriteStateCommand.Create(path, 6.0F),
            context,
            CancellationToken.None);
        var result = await reader.HandleAsync(
            new ReadStateCommand(path),
            context,
            CancellationToken.None);
        var rejectedStatus = await writer.HandleAsync(
            WriteStateCommand.Create(path, true),
            context,
            CancellationToken.None);

        Assert.Equal(StateWriteStatus.Applied, status);
        Assert.Equal(StateWriteStatus.Rejected, rejectedStatus);
        Assert.Equal(6.0F, Assert.IsType<float>(result));
        Assert.Equal(6.0F, instance.Dsp.InputGain.GainDb.Value);
        Assert.Equal(6.0F, instance.Runtime.Gain);

        fixture.DisposeInstance(instance);
    }

    [Fact]
    public void TopologyChangesRebuildPeersLimitsAndFocusedNotificationTargets()
    {
        var fixture = new StateFixture();
        var first = fixture.CreateInstance(new InstanceId(1));
        var second = fixture.CreateInstance(new InstanceId(2));
        var firstBank = new BankAddress(first.State.InstanceId, 0);
        var secondBank = new BankAddress(second.State.InstanceId, 5);
        first.State.FocusedBank = firstBank;
        second.State.FocusedBank = secondBank;

        var firstGain = first.Dsp.EqualizerBanks[0].Filters[0].GainDb;
        var secondGain = second.Dsp.EqualizerBanks[5].Filters[0].GainDb;
        secondGain.Value = 24.0F;

        var groupId = new GroupId(10);
        first.State.Banks[0].Group.Value = groupId;
        second.State.Banks[5].Group.Value = groupId;

        Assert.Throws<InvalidOperationException>(() => firstGain.Value = 1.0F);
        Assert.Equal(0.0F, firstGain.Value);
        Assert.Equal(24.0F, secondGain.Value);

        second.State.Banks[5].Group.Value = null;
        secondGain.Value = 0.0F;
        second.State.Banks[5].Group.Value = groupId;
        second.State.FocusedBank = firstBank;
        fixture.Changes.Clear();

        firstGain.Value = 5.0F;

        Assert.Equal(5.0F, firstGain.Value);
        Assert.Equal(5.0F, secondGain.Value);
        var firstBankChange = Assert.Single(
            fixture.Changes,
            change =>
                change.InstanceId == first.State.InstanceId &&
                change.Path.Nodes.Contains(StateNodeIds.BankAt(0)));
        var targets = fixture.NotificationRouter.ResolveTargets(firstBankChange);
        Assert.Equal(
            new[] { first.State.InstanceId.Value, second.State.InstanceId.Value },
            targets.OrderBy(value => value));

        fixture.DisposeInstance(first);
        fixture.DisposeInstance(second);
    }

    [Fact]
    public void FilterGainUsesItsPhysicalRangeWithoutNormalization()
    {
        var fixture = new StateFixture();
        var instance = fixture.CreateInstance(new InstanceId(1));
        var gain = instance.Dsp.EqualizerBanks[0].Filters[0].GainDb;

        gain.Value = -12.0F;

        Assert.Equal(-12.0F, gain.Value);
        Assert.Throws<InvalidOperationException>(() => gain.Value = -25.0F);
        Assert.Equal(-12.0F, gain.Value);

        fixture.DisposeInstance(instance);
    }

    [Fact]
    public void AudibilityFollowsObservedSoloAndTopologyChanges()
    {
        var fixture = new StateFixture();
        var first = fixture.CreateInstance(new InstanceId(1));
        var second = fixture.CreateInstance(new InstanceId(2));
        var groupId = new GroupId(10);
        first.State.Banks[0].Group.Value = groupId;
        second.State.Banks[5].Group.Value = groupId;

        first.State.Solo.Value = true;

        Assert.True(first.Runtime.Audible);
        Assert.False(second.Runtime.Audible);

        second.State.Banks[5].Group.Value = null;

        Assert.True(first.Runtime.Audible);
        Assert.True(second.Runtime.Audible);

        fixture.DisposeInstance(first);
        fixture.DisposeInstance(second);
    }

    private sealed class StateFixture : IStateChangeSink
    {
        private readonly TopologyIndex _topology = new();
        private readonly StatePeerObserver _peers;
        private readonly AudibilityObserver _audibility;
        private readonly StateTopologyObserver _topologyObserver;

        public StateFixture()
        {
            History = new StateHistory();
            _peers = new StatePeerObserver(History, _topology);
            _audibility = new AudibilityObserver(_topology);
            _topologyObserver = new StateTopologyObserver(
                _topology,
                _peers,
                _audibility);
            Storage = new StateRegistry<InstanceId>(History);
            Values = new StateValueFactory(Storage, _peers, this);
            NotificationRouter = new StateChangeRouter(_topology);
        }

        public StateHistory History { get; }

        public StateRegistry<InstanceId> Storage { get; }

        public StateValueFactory Values { get; }

        public StateChangeRouter NotificationRouter { get; }

        public List<StateValueChanged> Changes { get; } = new();

        public TestInstance CreateInstance(InstanceId instanceId)
        {
            Storage.CreateRoot(instanceId);
            var runtime = DspDefaults.CreateRuntime();
            var state = new InstanceState(
                instanceId,
                Values,
                runtime,
                _audibility,
                _topologyObserver);
            var dsp = new DspState(instanceId, Values, runtime);
            var managedState = new ManagedState(
                state,
                dsp,
                runtime,
                Storage.GetRoot(instanceId));
            _topologyObserver.AddState(state);
            return new TestInstance(state, dsp, runtime, managedState);
        }

        public void DisposeInstance(TestInstance instance)
        {
            Storage.RemoveRoot(instance.State.InstanceId);
            _topologyObserver.RemoveState(instance.State.InstanceId);
        }

        public void Publish(StateValueChanged change)
        {
            Changes.Add(change);
        }
    }

    private sealed record TestInstance(
        InstanceState State,
        DspState Dsp,
        DspRuntimeState Runtime,
        ManagedState ManagedState);
}
