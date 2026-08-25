using System;
using System.Diagnostics;
using System.Linq;
using System.Threading;

using Consolidator.Managed.Core.Services.Abstractions;
using Consolidator.Managed.Protocol.Messages;
using Consolidator.Managed.Tests.Support;
using Xunit;

namespace Consolidator.Managed.Tests.UseCases;

using static ManagedApplicationFixture;

public sealed class StateEditingUseCasesTests
{
    [Fact]
    public void WriteReadAndResetFlowUpdatesStateNotificationsAndDspProjection()
    {
        using var application = new ManagedApplicationFixture();
        var instance = application.RegisterInstance();
        instance.Output.Clear();
        var initialPublishCount = instance.Dsp.PublishCount;

        application.Send(
            instance,
            "write",
            Symbol(instance.InstanceId.Value.ToString()),
            Symbol("0"),
            Integer(1),
            Symbol("entry"),
            Symbol("input_gain"),
            Symbol("gain"),
            Symbol("value"),
            Float(6.0));

        Assert.Equal(6.0F, instance.Dsp.Latest.Gain);
        Assert.True(instance.Dsp.PublishCount > initialPublishCount);
        Assert.Equal(1, instance.Output.Single("action_done").Atoms[^1].Integer);
        var change = instance.Output.Single("state_changed");
        Assert.Equal("input_gain.gain", change.Atoms[1].Symbol);
        Assert.Equal(6.0, change.Atoms[2].Float);

        instance.Output.Clear();
        application.Send(
            instance,
            "read",
            Integer(1),
            Symbol("query"),
            Symbol("input_gain"),
            Symbol("gain"));

        Assert.Equal(6.0, instance.Output.Single("state_done").Atoms[^1].Float);

        instance.Output.Clear();
        application.Send(
            instance,
            "reset",
            Symbol(instance.InstanceId.Value.ToString()),
            Symbol("0"),
            Symbol("input_gain"));

        Assert.Equal(1.0F, instance.Dsp.Latest.Gain);
        Assert.Equal(1, instance.Output.Single("action_done").Atoms[^1].Integer);
        Assert.Contains(
            instance.Output.Messages,
            message => message.Selector == "state_changed" &&
                message.Atoms[1].Symbol == "input_gain.gain" &&
                message.Atoms[2].Float == 1.0);
    }

    [Fact]
    public void MalformedWriteReturnsProtocolErrorWithoutChangingDspState()
    {
        using var application = new ManagedApplicationFixture();
        var instance = application.RegisterInstance();
        instance.Output.Clear();
        var initial = instance.Dsp.Latest.Gain;
        var publishCount = instance.Dsp.PublishCount;

        application.Send(
            instance,
            "write",
            Symbol(instance.InstanceId.Value.ToString()),
            Symbol("0"),
            Integer(1),
            Symbol("entry"),
            Symbol("input_gain"),
            Symbol("gain"));

        Assert.Equal(initial, instance.Dsp.Latest.Gain);
        Assert.Equal(publishCount, instance.Dsp.PublishCount);
        Assert.Equal("error", Assert.Single(instance.Output.Messages).Selector);
    }

    [Fact]
    public void DetectorFilterGainWriteUpdatesTheFilterGainState()
    {
        using var application = new ManagedApplicationFixture();
        var instance = application.RegisterInstance();
        instance.Output.Clear();

        application.Send(
            instance,
            "write",
            Symbol(instance.InstanceId.Value.ToString()),
            Symbol("0"),
            Integer(1),
            Symbol("entry"),
            Symbol("compressor"),
            Symbol("detector"),
            Symbol("filter"),
            Integer(1),
            Symbol("gain"),
            Symbol("value"),
            Float(6.0));

        Assert.True(SpinWait.SpinUntil(
            () => instance.Output.Messages.Count >= 2,
            TimeSpan.FromSeconds(1)));
        var messages = instance.Output.Messages.ToArray();
        Assert.Contains(messages, message => message.Selector == "action_done");
        var change = Assert.Single(messages, message => message.Selector == "state_changed");
        Assert.Equal("compressor.detector.filter.1.gain", change.Atoms[1].Symbol);
        Assert.Equal(6.0, change.Atoms[2].Float);
    }

    [Fact]
    public void ConnectedControlWritePublishesEveryAffectedDspState()
    {
        using var application = new ManagedApplicationFixture();
        var first = application.RegisterInstance();
        var second = application.RegisterInstance();
        application.Send(
            first,
            "observe_target",
            Symbol(first.InstanceId.Value.ToString()),
            Integer(7));
        first.Output.Clear();
        second.Output.Clear();
        var firstPublishCount = first.Dsp.PublishCount;
        var secondPublishCount = second.Dsp.PublishCount;

        application.Send(
            first,
            "write",
            Symbol(first.InstanceId.Value.ToString()),
            Symbol("0"),
            Integer(1),
            Symbol("entry"),
            Symbol("compressor"),
            Symbol("threshold"),
            Symbol("value"),
            Float(-18.0));

        Assert.True(SpinWait.SpinUntil(
            () => first.Dsp.PublishCount > firstPublishCount &&
                second.Dsp.PublishCount > secondPublishCount,
            TimeSpan.FromSeconds(1)));
        Assert.Equal(-18.0F, first.Dsp.Latest.CompressorThresholdDb);
        Assert.Equal(-18.0F, second.Dsp.Latest.CompressorThresholdDb);
    }

    [Fact]
    public void AddingPeerPublishesChangedEffectiveRangeToExistingControl()
    {
        using var application = new ManagedApplicationFixture();
        var first = application.RegisterInstance();
        application.Send(
            first,
            "observe_target",
            Symbol(first.InstanceId.Value.ToString()),
            Integer(7));

        application.Send(
            first,
            "write",
            Symbol(first.InstanceId.Value.ToString()),
            Symbol("0"),
            Integer(1),
            Symbol("entry"),
            Symbol("compressor"),
            Symbol("threshold"),
            Symbol("value"),
            Float(-1.0));
        first.Output.Clear();

        var second = application.RegisterInstance();

        var rangeChange = Assert.Single(
            first.Output.Messages,
            message => message.Selector == "state_changed" &&
                message.Atoms[1].Symbol ==
                    "compressor.threshold");
        Assert.Equal(-1.0, rangeChange.Atoms[2].Float);
        Assert.Equal(-97.0, rangeChange.Atoms[6].Float);
        Assert.Equal(0.0, rangeChange.Atoms[7].Float);

        second.Output.Clear();
        application.Send(
            second,
            "observe_target",
            Symbol(first.InstanceId.Value.ToString()),
            Integer(7));
        var threshold = Assert.Single(
            second.Output.Messages,
            message => message.Selector == "target_state_entry" &&
                message.Atoms[4].Symbol == "compressor.threshold");
        Assert.Equal(-97.0, threshold.Atoms[9].Float);
        Assert.Equal(0.0, threshold.Atoms[10].Float);
    }

    [Fact]
    public void BankFocusUsesSnapshotWithoutIntermediateRangeNotifications()
    {
        using var application = new ManagedApplicationFixture();
        var instances = Enumerable.Range(0, 6)
            .Select(_ => application.RegisterInstance())
            .ToArray();
        var source = instances[0];
        application.Send(
            source,
            "observe_target",
            Symbol(source.InstanceId.Value.ToString()),
            Integer(1));
        application.Send(
            source,
            "write",
            Symbol(source.InstanceId.Value.ToString()),
            Symbol("0"),
            Integer(1),
            Symbol("entry"),
            Symbol("compressor"),
            Symbol("threshold"),
            Symbol("value"),
            Float(-1.0));
        source.Output.Clear();

        application.Send(
            source,
            "observe_target",
            Symbol(source.InstanceId.Value.ToString()),
            Integer(7));

        Assert.DoesNotContain(
            source.Output.Messages,
            message => message.Selector == "state_changed");
        Assert.Contains(
            source.Output.Messages,
            message => message.Selector == "target_state_entry" &&
                message.Atoms[4].Symbol == "compressor.threshold" &&
                message.Atoms[9].Float == -97.0 &&
                message.Atoms[10].Float == 0.0);
    }

    [Fact]
    public void StateChangeRangesUseEachObserversSelectedBankContext()
    {
        using var application = new ManagedApplicationFixture();
        var target = application.RegisterInstance();
        var groupedPeer = application.RegisterInstance();
        var localObserver = application.RegisterInstance();
        application.Send(
            target,
            "observe_target",
            Symbol(target.InstanceId.Value.ToString()),
            Integer(7));
        application.Send(
            localObserver,
            "observe_target",
            Symbol(target.InstanceId.Value.ToString()),
            Integer(1));
        target.Output.Clear();
        groupedPeer.Output.Clear();
        localObserver.Output.Clear();

        application.Send(
            localObserver,
            "write",
            Symbol(target.InstanceId.Value.ToString()),
            Symbol("0"),
            Integer(1),
            Symbol("entry"),
            Symbol("compressor"),
            Symbol("threshold"),
            Symbol("value"),
            Float(-1.0));

        var groupedChange = target.Output.Single("state_changed");
        var localChange = localObserver.Output.Single("state_changed");
        Assert.Equal(-97.0, groupedChange.Atoms[6].Float);
        Assert.Equal(-120.0, localChange.Atoms[6].Float);
        Assert.Equal(0.0, groupedChange.Atoms[7].Float);
        Assert.Equal(0.0, localChange.Atoms[7].Float);
    }

    [Fact]
    public void DetectorFilterEditFollowsTheSelectedBanksGroup()
    {
        using var application = new ManagedApplicationFixture();
        var first = application.RegisterInstance();
        var second = application.RegisterInstance();
        first.Output.Clear();
        second.Output.Clear();

        application.Send(
            first,
            "write",
            Symbol(first.InstanceId.Value.ToString()),
            Symbol("0"),
            Integer(1),
            Symbol("entry"),
            Symbol("compressor"),
            Symbol("detector"),
            Symbol("filter"),
            Integer(1),
            Symbol("frequency"),
            Symbol("value"),
            Float(2000.0));

        application.Send(
            second,
            "read",
            Integer(1),
            Symbol("query"),
            Symbol("compressor"),
            Symbol("detector"),
            Symbol("filter"),
            Integer(1),
            Symbol("frequency"));

        Assert.Equal(1000.0, second.Output.Single("state_done").Atoms[^1].Float);
        Assert.DoesNotContain(
            second.Output.Messages,
            message => message.Selector == "state_changed" &&
                message.Atoms[1].Symbol ==
                    "compressor.detector.filter.1.frequency");

        application.Send(
            second,
            "observe_target",
            Symbol(first.InstanceId.Value.ToString()),
            Integer(7));
        second.Output.Clear();
        application.Send(
            second,
            "write",
            Symbol(first.InstanceId.Value.ToString()),
            Symbol("0"),
            Integer(1),
            Symbol("entry"),
            Symbol("compressor"),
            Symbol("detector"),
            Symbol("filter"),
            Integer(1),
            Symbol("frequency"),
            Symbol("value"),
            Float(3000.0));

        application.Send(
            second,
            "observe_target",
            Symbol(second.InstanceId.Value.ToString()),
            Integer(1));
        second.Output.Clear();
        application.Send(
            second,
            "read",
            Integer(1),
            Symbol("query"),
            Symbol("compressor"),
            Symbol("detector"),
            Symbol("filter"),
            Integer(1),
            Symbol("frequency"));

        Assert.Equal(2000.0, second.Output.Single("state_done").Atoms[^1].Float);
    }

    [Fact]
    public void AnalyzerActivationPublishesTheCurrentBypassedCurve()
    {
        using var application = new ManagedApplicationFixture();
        var instance = application.RegisterInstance();
        instance.Output.Clear();

        application.Send(
            instance,
            "write",
            Symbol(instance.InstanceId.Value.ToString()),
            Symbol("0"),
            Integer(1),
            Symbol("entry"),
            Symbol("equalizer"),
            Symbol("bank"),
            Integer(1),
            Symbol("bypass"),
            Symbol("value"),
            Integer(1));

        Assert.Equal(
            1,
            instance.Output.Single("action_done").Atoms[^1].Integer);
        Assert.DoesNotContain(
            instance.Output.Messages,
            message => message.Selector == "equalizer_curves");

        instance.Output.Clear();
        application.Send(
            instance,
            "set_instance_active",
            Integer(1));
        Assert.True(SpinWait.SpinUntil(
            () => instance.Output.Messages.Any(message =>
                message.Selector == "equalizer_curves" &&
                message.Atoms[1].Integer == 0),
            TimeSpan.FromSeconds(1)));
    }

    [Fact]
    public void GroupedDetectorEditPublishesCurvesOnlyToActiveAnalyzers()
    {
        using var application = new ManagedApplicationFixture();
        var instances = Enumerable.Range(0, 6)
            .Select(_ => application.RegisterInstance())
            .ToArray();
        var active = instances[0];

        application.Send(
            active,
            "observe_target",
            Symbol(active.InstanceId.Value.ToString()),
            Integer(7));
        application.Send(
            active,
            "set_instance_active",
            Integer(1));
        foreach (var instance in instances)
        {
            instance.Output.Clear();
        }

        application.Send(
            active,
            "write",
            Symbol(active.InstanceId.Value.ToString()),
            Symbol("0"),
            Integer(1),
            Symbol("entry"),
            Symbol("compressor"),
            Symbol("detector"),
            Symbol("filter"),
            Integer(1),
            Symbol("frequency"),
            Symbol("value"),
            Float(2000.0));

        Assert.True(SpinWait.SpinUntil(
            () => active.Output.Messages.Any(message =>
                message.Selector == "compressor_detector_curves"),
            TimeSpan.FromSeconds(1)));
        foreach (var inactive in instances.Skip(1))
        {
            Assert.DoesNotContain(
                inactive.Output.Messages,
                message => message.Selector.EndsWith("_curves"));
        }
    }

    [Fact]
    public void ActivatingAnotherInstanceTransfersAnalyzerDelivery()
    {
        using var application = new ManagedApplicationFixture();
        var first = application.RegisterInstance();
        var second = application.RegisterInstance();
        foreach (var instance in new[] { first, second })
        {
            application.Send(
                instance,
                "observe_target",
                Symbol(instance.InstanceId.Value.ToString()),
                Integer(7));
            application.Send(
                instance,
                "set_instance_active",
                Integer(1));
            instance.Output.Clear();
        }

        application.Send(
            first,
            "write",
            Symbol(first.InstanceId.Value.ToString()),
            Symbol("0"),
            Integer(1),
            Symbol("entry"),
            Symbol("compressor"),
            Symbol("detector"),
            Symbol("filter"),
            Integer(1),
            Symbol("frequency"),
            Symbol("value"),
            Float(2000.0));

        Assert.True(SpinWait.SpinUntil(
            () => second.Output.Messages.Any(message =>
                message.Selector == "compressor_detector_curves"),
            TimeSpan.FromSeconds(1)));
        Assert.DoesNotContain(
            first.Output.Messages,
            message => message.Selector.EndsWith("_curves"));
    }

    [Fact]
    public void GroupedDetectorEditPrioritizesTheObservedCurve()
    {
        using var application = new ManagedApplicationFixture();
        var instances = Enumerable.Range(0, 48)
            .Select(_ => application.RegisterInstance())
            .ToArray();
        var observer = instances[0];
        var writer = instances[1];
        var observedSource = instances[^1];

        application.Send(
            observer,
            "observe_target",
            Symbol(observedSource.InstanceId.Value.ToString()),
            Integer(7));
        application.Send(
            observer,
            "set_instance_active",
            Integer(1));
        application.Send(
            writer,
            "observe_target",
            Symbol(writer.InstanceId.Value.ToString()),
            Integer(7));
        foreach (var instance in instances)
        {
            instance.Output.Clear();
        }

        var startedAt = Stopwatch.StartNew();
        application.Send(
            writer,
            "write",
            Symbol(writer.InstanceId.Value.ToString()),
            Symbol("0"),
            Integer(1),
            Symbol("entry"),
            Symbol("compressor"),
            Symbol("detector"),
            Symbol("filter"),
            Integer(1),
            Symbol("frequency"),
            Symbol("value"),
            Float(2000.0));

        Assert.True(SpinWait.SpinUntil(
            () => observer.Output.Messages.Any(message =>
                    message.Selector == "compressor_detector_curves"),
            TimeSpan.FromMilliseconds(500)));
        Assert.True(startedAt.Elapsed < TimeSpan.FromMilliseconds(500));
    }

    [Fact]
    public unsafe void ActiveInstanceReceivesSpectrumFromItsObservedSource()
    {
        using var application = new ManagedApplicationFixture();
        var viewer = application.RegisterInstance();
        var source = application.RegisterInstance();
        var audio = application
            .GetRequiredService<IInstanceAudioInputService>();
        var samples = new double[4096];

        application.Send(
            viewer,
            "observe_target",
            Symbol(source.InstanceId.Value.ToString()),
            Integer(1));
        application.Send(
            viewer,
            "set_instance_active",
            Integer(1));
        viewer.Output.Clear();
        source.Output.Clear();

        fixed (double* samplePointer = samples)
        {
            audio.ReceiveAudio(
                source.InstanceId,
                samplePointer,
                samplePointer,
                samplePointer,
                samplePointer,
                (nuint)samples.Length);
        }

        Assert.True(SpinWait.SpinUntil(
            () => viewer.Output.Messages.Any(message =>
                message.Selector == "fft"),
            TimeSpan.FromSeconds(1)));
        Assert.Equal(
            (long)source.InstanceId.Value,
            viewer.Output.Single("fft").Atoms[1].Integer);
        Assert.DoesNotContain(
            source.Output.Messages,
            message => message.Selector == "fft");
    }

    [Fact]
    public void RejectedMultiValueControlWriteDoesNotCommitAnyEntry()
    {
        using var application = new ManagedApplicationFixture();
        var instance = application.RegisterInstance();
        instance.Output.Clear();

        application.Send(
            instance,
            "write",
            Symbol(instance.InstanceId.Value.ToString()),
            Symbol("0"),
            Integer(2),
            Symbol("entry"),
            Symbol("compressor"),
            Symbol("detector"),
            Symbol("filter"),
            Integer(1),
            Symbol("frequency"),
            Symbol("value"),
            Float(2000.0),
            Symbol("entry"),
            Symbol("compressor"),
            Symbol("detector"),
            Symbol("filter"),
            Integer(1),
            Symbol("gain"),
            Symbol("value"),
            Float(100.0));

        Assert.True(SpinWait.SpinUntil(
            () => instance.Output.Messages.Any(message =>
                message.Selector == "error"),
            TimeSpan.FromSeconds(1)));
        Assert.DoesNotContain(
            instance.Output.Messages,
            message => message.Selector == "state_changed");

        instance.Output.Clear();
        application.Send(
            instance,
            "read",
            Integer(1),
            Symbol("query"),
            Symbol("compressor"),
            Symbol("detector"),
            Symbol("filter"),
            Integer(1),
            Symbol("frequency"));

        Assert.True(SpinWait.SpinUntil(
            () => instance.Output.Messages.Any(message =>
                message.Selector == "state_done"),
            TimeSpan.FromSeconds(1)));
        Assert.Equal(1000.0, instance.Output.Single("state_done").Atoms[^1].Float);
    }
}
