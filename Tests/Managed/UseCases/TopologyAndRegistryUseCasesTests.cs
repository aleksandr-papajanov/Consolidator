using System;
using System.Collections.Generic;
using System.Linq;

using Consolidator.Managed.Tests.Support;
using Xunit;

namespace Consolidator.Managed.Tests.UseCases;

using static ManagedApplicationFixture;

public sealed class TopologyAndRegistryUseCasesTests
{
    [Fact]
    public void UnregisterDisposesObservedPeerValuesExactlyOnce()
    {
        using var application = new ManagedApplicationFixture();
        var source = application.RegisterInstance();
        var target = application.RegisterInstance();

        application.Send(
            source,
            "observe_target",
            Symbol(target.InstanceId.Value.ToString()),
            Integer(3),
            Symbol("equalizer"));
        application.Send(source, "set_instance_active", Integer(1));

        application.UnregisterInstance(source);
        application.UnregisterInstance(source);
    }

    [Fact]
    public void GroupedBanksPropagateEditsAndRegistryReportsTheConnectedTopology()
    {
        using var application = new ManagedApplicationFixture();
        var first = application.RegisterInstance();
        var second = application.RegisterInstance();
        first.Output.Clear();
        second.Output.Clear();

        WriteGroup(application, first, first, 0, 7);
        WriteGroup(application, first, second, 1, 7);
        application.Send(
            first,
            "observe_target",
            Symbol(first.InstanceId.Value.ToString()),
            Integer(0),
            Symbol("equalizer"));
        application.Send(
            second,
            "observe_target",
            Symbol(second.InstanceId.Value.ToString()),
            Integer(1),
            Symbol("equalizer"));
        application.Send(first, "set_instance_active", Integer(1));
        first.Output.Clear();
        second.Output.Clear();

        WriteFilterGain(application, first, first, 0, 4.5);

        Assert.Contains(
            first.Output.Messages,
            message => IsFilterGainChange(message, 4.5));
        Assert.DoesNotContain(
            second.Output.Messages,
            message => IsFilterGainChange(message, 4.5));
        application.Send(
            second,
            "read",
            Integer(1),
            Symbol("query"),
            Symbol("equalizer"),
            Symbol("bank"),
            Integer(1),
            Symbol("filter"),
            Integer(1),
            Symbol("gain"));
        Assert.Equal(4.5, second.Output.Single("state_done").Atoms[^1].Float);

        first.Output.Clear();
        application.Send(first, "registry");
        var messages = first.Output.Messages;
        Assert.Equal(2, messages.Count(message => message.Selector == "registry_instance"));
        Assert.Contains(
            messages,
                message => message.Selector == "registry_member" &&
                message.Atoms[3].Integer == 7 &&
                message.Atoms[4].Symbol == first.InstanceId.Value.ToString() &&
                message.Atoms[5].Integer == 0);
        Assert.Contains(
            messages,
                message => message.Selector == "registry_member" &&
                message.Atoms[3].Integer == 7 &&
                message.Atoms[4].Symbol == second.InstanceId.Value.ToString() &&
                message.Atoms[5].Integer == 1);
    }

    [Fact]
    public void ProcessorMarkersFollowExactTopologyAndViewerFocus()
    {
        using var application = new ManagedApplicationFixture();
        var viewer = application.RegisterInstance();
        var remote = application.RegisterInstance();

        WriteFilterGain(application, remote, remote, 1, 4.5);
        application.Send(
            viewer,
            "observe_target",
            Symbol(viewer.InstanceId.Value.ToString()),
            Integer(0),
            Symbol("equalizer"));
        application.Send(viewer, "registry");

        var initialMarker = viewer.Output.Messages.Single(message =>
            message.Selector == "registry_processor" &&
            message.Atoms[3].Symbol == viewer.InstanceId.Value.ToString() &&
            message.Atoms[4].Symbol == "equalizer");
        Assert.Equal(0, initialMarker.Atoms[6].Integer);

        viewer.Output.Clear();
        WriteGroup(application, viewer, viewer, 0, 7);
        WriteGroup(application, viewer, remote, 1, 7);

        Assert.DoesNotContain(
            viewer.Output.Messages,
            message => IsProcessorMarkerChange(
                message,
                viewer.InstanceId.Value,
                "equalizer",
                true));

        viewer.Output.Clear();
        application.Send(
            viewer,
            "observe_target",
            Symbol(remote.InstanceId.Value.ToString()),
            Integer(1),
            Symbol("equalizer"));

        Assert.Contains(
            viewer.Output.Messages,
            message => IsProcessorMarkerChange(
                message,
                remote.InstanceId.Value,
                "equalizer",
                true));
    }

    [Fact]
    public void SoloUsesExplicitBankGroupForExclusiveAndAdditiveSelection()
    {
        using var application = new ManagedApplicationFixture();
        var first = application.RegisterInstance();
        var second = application.RegisterInstance();
        var third = application.RegisterInstance();

        WriteGroup(application, first, first, 0, 7);
        WriteGroup(application, first, second, 1, 7);
        application.Send(
            first,
            "observe_target",
            Symbol(first.InstanceId.Value.ToString()),
            Integer(0),
            Symbol("equalizer"));
        application.Send(
            second,
            "observe_target",
            Symbol(second.InstanceId.Value.ToString()),
            Integer(1),
            Symbol("equalizer"));
        application.Send(
            third,
            "observe_target",
            Symbol(third.InstanceId.Value.ToString()),
            Integer(0),
            Symbol("equalizer"));

        application.Send(first, "registry");
        application.Send(second, "registry");
        application.Send(third, "registry");
        first.Output.Clear();
        second.Output.Clear();
        third.Output.Clear();

        SendSolo(application, first, first, 0, true, false, true);

        AssertSolo(application, first, true);
        AssertSolo(application, second, true);
        AssertSolo(application, third, false);

        first.Output.Clear();
        second.Output.Clear();
        third.Output.Clear();
        SendSolo(application, third, third, null, true, false, false);

        AssertSolo(application, first, false);
        AssertSolo(application, second, false);
        AssertSolo(application, third, true);

        first.Output.Clear();
        second.Output.Clear();
        third.Output.Clear();
        SendSolo(application, first, first, 0, true, true, true);

        AssertSolo(application, first, true);
        AssertSolo(application, second, true);
        AssertSolo(application, third, true);

        first.Output.Clear();
        second.Output.Clear();
        third.Output.Clear();
        SendSolo(application, first, first, 0, false, true, true);

        AssertSolo(application, first, false);
        AssertSolo(application, second, false);
        AssertSolo(application, third, true);
    }

    [Fact]
    public void AdditiveSoloAddsTheCompleteGroupWhenFocusedBankChangesGroups()
    {
        using var application = new ManagedApplicationFixture();
        var trackA = application.RegisterInstance();
        var trackB = application.RegisterInstance();
        var trackC = application.RegisterInstance();
        var trackD = application.RegisterInstance();

        WriteGroup(application, trackA, trackA, 0, 1);
        WriteGroup(application, trackA, trackC, 0, 1);
        WriteGroup(application, trackB, trackB, 0, 2);
        WriteGroup(application, trackB, trackD, 0, 2);

        SendSolo(application, trackA, trackA, 0, true, false, true);
        SendSolo(application, trackB, trackB, 0, true, true, true);

        AssertSolo(application, trackA, true);
        AssertSolo(application, trackB, true);
        AssertSolo(application, trackC, true);
        AssertSolo(application, trackD, true);
    }

    [Fact]
    public void GroupScopeTreatsAnUngroupedTargetAsAStandaloneInstance()
    {
        using var application = new ManagedApplicationFixture();
        var grouped = application.RegisterInstance();
        var groupPeer = application.RegisterInstance();
        var ungrouped = application.RegisterInstance();

        WriteGroup(application, grouped, grouped, 0, 1);
        WriteGroup(application, grouped, groupPeer, 0, 1);

        SendSolo(application, grouped, grouped, 0, true, false, true);
        SendSolo(application, grouped, ungrouped, 0, true, false, true);

        AssertSolo(application, grouped, false);
        AssertSolo(application, groupPeer, false);
        AssertSolo(application, ungrouped, true);
    }

    [Fact]
    public void SoloRecognizesAGroupWhenANewTrackJoinsAfterTheGroupExists()
    {
        using var application = new ManagedApplicationFixture();
        var first = application.RegisterInstance();
        WriteGroup(application, first, first, 0, 7);

        var newTrack = application.RegisterInstance();
        WriteGroup(application, first, newTrack, 1, 7);
        application.Send(
            first,
            "observe_target",
            Symbol(first.InstanceId.Value.ToString()),
            Integer(0),
            Symbol("equalizer"));
        application.Send(
            newTrack,
            "observe_target",
            Symbol(newTrack.InstanceId.Value.ToString()),
            Integer(1),
            Symbol("equalizer"));
        application.Send(first, "registry");
        application.Send(newTrack, "registry");
        first.Output.Clear();
        newTrack.Output.Clear();

        SendSolo(application, first, first, 0, true, false, true);

        AssertSolo(application, first, true);
        AssertSolo(application, newTrack, true);
    }

    [Fact]
    public void InstanceControlsUseExplicitInstanceOrGroupTargets()
    {
        using var application = new ManagedApplicationFixture();
        var track1 = application.RegisterInstance();
        var track2 = application.RegisterInstance();
        var track3 = application.RegisterInstance();
        var track4 = application.RegisterInstance();

        application.Send(
            track4,
            "observe_target",
            Symbol(track4.InstanceId.Value.ToString()),
            Integer(6),
            Symbol("equalizer"));
        foreach (var instance in new[] { track1, track2, track3, track4 })
        {
            application.Send(instance, "registry");
            instance.Output.Clear();
        }

        SendSolo(application, track4, track2, null, true, false, false);

        AssertSolo(application, track1, false);
        AssertSolo(application, track2, true);
        AssertSolo(application, track3, false);
        AssertSolo(application, track4, false);

        SendMute(application, track4, track2, 6, true, true);

        AssertMute(application, track1, true);
        AssertMute(application, track2, true);
        AssertMute(application, track3, true);
        AssertMute(application, track4, true);
    }

    [Fact]
    public void GroupInstanceControlDoesNotTraverseAnotherGroupOnTheSameTrack()
    {
        using var application = new ManagedApplicationFixture();
        var first = application.RegisterInstance();
        var bridge = application.RegisterInstance();
        var third = application.RegisterInstance();

        WriteGroup(application, first, first, 0, 7);
        WriteGroup(application, first, bridge, 1, 7);
        WriteGroup(application, first, bridge, 2, 8);
        WriteGroup(application, first, third, 3, 8);

        SendSolo(application, first, first, 0, true, false, true);

        AssertSolo(application, first, true);
        AssertSolo(application, bridge, true);
        AssertSolo(application, third, false);
    }

    [Fact]
    public void UngroupedInstanceControlTargetFallsBackToTheInstance()
    {
        using var application = new ManagedApplicationFixture();
        var instance = application.RegisterInstance();

        SendMute(application, instance, instance, 0, true, true);

        AssertMute(application, instance, true);
    }

    [Fact]
    public void TopologyChangesDoNotRewriteInstanceControlValues()
    {
        using var application = new ManagedApplicationFixture();
        var first = application.RegisterInstance();
        var second = application.RegisterInstance();

        WriteGroup(application, first, first, 0, 7);
        WriteGroup(application, first, second, 1, 7);
        SendMute(application, first, first, 0, true, true);

        WriteGroup(application, first, second, 1, null);

        AssertMute(application, first, true);
        AssertMute(application, second, true);
    }

    [Fact]
    public void DirectStateWriteCannotBypassInstanceControlPolicy()
    {
        using var application = new ManagedApplicationFixture();
        var instance = application.RegisterInstance();

        application.Send(
            instance,
            "write",
            Symbol("group"),
            Symbol("0"),
            Integer(2),
            Symbol("entry"),
            Symbol("mute"),
            Symbol("value"),
            Integer(1),
            Symbol("entry"),
            Symbol("solo"),
            Symbol("value"),
            Integer(1));

        AssertMute(application, instance, false);
        AssertSolo(application, instance, false);
    }

    [Fact]
    public void ObserveTargetReturnsACompleteBankRelativeSnapshot()
    {
        using var application = new ManagedApplicationFixture();
        var source = application.RegisterInstance();
        var target = application.RegisterInstance();
        source.Output.Clear();

        application.Send(
            source,
            "observe_target",
            Symbol(target.InstanceId.Value.ToString()),
            Integer(3),
            Symbol("equalizer"));

        var snapshot = source.Output.Single("target_state_snapshot");
        var entryCount = snapshot.Atoms[6].Integer;
        Assert.Equal(target.InstanceId.Value.ToString(), snapshot.Atoms[3].Symbol);
        Assert.Equal(3, snapshot.Atoms[4].Integer);
        Assert.Equal("equalizer", snapshot.Atoms[5].Symbol);
        Assert.True(entryCount > 0);
        Assert.Contains(
            Enumerable.Range(0, (int)entryCount),
            index => snapshot.Atoms[7 + index * 6].Symbol == "equalizer.filter.1.gain");
    }

    private static void WriteGroup(
        ManagedApplicationFixture application,
        TestInstance source,
        TestInstance target,
        int bank,
        int? group)
    {
        application.Send(
            source,
            "write",
            Symbol("topology"),
            Symbol(target.InstanceId.Value.ToString()),
            Symbol("0"),
            Integer(1),
            Symbol("entry"),
            Symbol("bank"),
            Integer(bank),
            Symbol("group"),
            Symbol("value"),
            group is { } groupId
                ? Integer(groupId)
                : Symbol("none"));
    }

    private static void SendSolo(
        ManagedApplicationFixture application,
        TestInstance source,
        TestInstance target,
        int? bankId,
        bool enabled,
        bool additive,
        bool group)
    {
        application.Send(
            source,
            "observe_target",
            Symbol(target.InstanceId.Value.ToString()),
            Integer(bankId ?? 0),
            Symbol("equalizer"));
        application.Send(source, "set_instance_solo", new[]
        {
            Symbol(target.InstanceId.Value.ToString()),
            Symbol(group ? "group" : "local"),
            Integer(enabled ? 1 : 0),
            Symbol(additive ? "additive" : "exclusive")
        });
    }

    private static void SendMute(
        ManagedApplicationFixture application,
        TestInstance source,
        TestInstance target,
        int? bankId,
        bool muted,
        bool group,
        bool additive = false)
    {
        application.Send(
            source,
            "observe_target",
            Symbol(target.InstanceId.Value.ToString()),
            Integer(bankId ?? 0),
            Symbol("equalizer"));
        application.Send(
            source,
            "set_instance_mute",
            Symbol(target.InstanceId.Value.ToString()),
            Symbol(group ? "group" : "local"),
            Integer(muted ? 1 : 0),
            Symbol(additive ? "additive" : "exclusive"));
    }

    private static void AssertSolo(
        ManagedApplicationFixture application,
        TestInstance instance,
        bool expected)
    {
        application.Send(instance, "registry");
        var snapshot = instance.Output.Messages.Last(message =>
            message.Selector == "registry_instance" &&
            message.Atoms[3].Symbol == instance.InstanceId.Value.ToString());
        Assert.Equal(expected ? 1 : 0, snapshot.Atoms[6].Integer);
    }

    private static void AssertMute(
        ManagedApplicationFixture application,
        TestInstance instance,
        bool expected)
    {
        application.Send(instance, "registry");
        var snapshot = instance.Output.Messages.Last(message =>
            message.Selector == "registry_instance" &&
            message.Atoms[3].Symbol == instance.InstanceId.Value.ToString());
        Assert.Equal(expected ? 1 : 0, snapshot.Atoms[5].Integer);
    }

    private static void WriteFilterGain(
        ManagedApplicationFixture application,
        TestInstance source,
        TestInstance target,
        int bank,
        double gain)
    {
        application.Send(
            source,
            "write",
            Symbol("group"),
            Symbol("0"),
            Integer(1),
            Symbol("entry"),
            Symbol("equalizer"),
            Symbol("bank"),
            Integer(bank),
            Symbol("filter"),
            Integer(1),
            Symbol("gain"),
            Symbol("value"),
            Float(gain));
    }

    private static bool IsFilterGainChange(
        Consolidator.Managed.Protocol.Messages.ProtocolOutput message,
        double expected) =>
        message.Selector == "state_changed" &&
        message.Atoms[1].Symbol == "equalizer.filter.1.gain" &&
        message.Atoms[2].Float == expected;

    private static bool IsProcessorMarkerChange(
        Consolidator.Managed.Protocol.Messages.ProtocolOutput message,
        ulong instanceId,
        string processorId,
        bool active) =>
        message.Selector == "registry_processor_markers_changed" &&
        ContainsProcessorMarker(message.Atoms, instanceId, processorId, active);

    private static bool ContainsProcessorMarker(
        IReadOnlyList<Consolidator.Managed.Protocol.Messages.Atom> atoms,
        ulong instanceId,
        string processorId,
        bool active)
    {
        var position = 2;
        for (var instanceIndex = 0; instanceIndex < atoms[1].Integer; instanceIndex++)
        {
            var currentInstanceId = atoms[position].Symbol;
            var processorCount = (int)atoms[position + 1].Integer;
            position += 2;
            for (var processorIndex = 0; processorIndex < processorCount; processorIndex++)
            {
                if (currentInstanceId == instanceId.ToString() &&
                    atoms[position].Symbol == processorId &&
                    atoms[position + 1].Integer == (active ? 1 : 0))
                {
                    return true;
                }
                position += 2;
            }
        }
        return false;
    }
}
