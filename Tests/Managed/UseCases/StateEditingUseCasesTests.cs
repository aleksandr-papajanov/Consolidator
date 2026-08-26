using System;
using System.Diagnostics;
using System.Linq;
using System.Threading;

using Consolidator.Managed.Core.Services.Abstractions;
using Consolidator.Managed.Protocol.Messages;
using Consolidator.Managed.Tests.Support;
using Xunit;
using Xunit.Abstractions;

namespace Consolidator.Managed.Tests.UseCases;

using static ManagedApplicationFixture;

public sealed class StateEditingUseCasesTests
{
    private readonly ITestOutputHelper _output;

    public StateEditingUseCasesTests(ITestOutputHelper output)
    {
        _output = output;
    }

    [Fact]
    public void WriteReadAndResetFlowUpdatesStateNotificationsAndDspProjection()
    {
        using var application = new ManagedApplicationFixture();
        var instance = application.RegisterInstance();
        application.Send(
            instance,
            "observe_target",
            Symbol(instance.InstanceId.Value.ToString()),
            Integer(0));
        application.Send(instance, "set_instance_active", Integer(1));
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
        var change = instance.Output.Messages
            .Last(message => message.Selector == "state_changed" &&
                message.Atoms[1].Symbol == "input_gain.gain");
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
        application.Send(
            instance,
            "observe_target",
            Symbol(instance.InstanceId.Value.ToString()),
            Integer(0));
        application.Send(instance, "set_instance_active", Integer(1));
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
    public void RejectedCallbacklessGestureWriteRetainsItsErrorResponse()
    {
        using var application = new ManagedApplicationFixture();
        var instance = application.RegisterInstance();
        instance.Output.Clear();

        var requestId = application.Enqueue(
            instance,
            "write",
            Symbol(instance.InstanceId.Value.ToString()),
            Symbol("42"),
            Integer(1),
            Symbol("entry"),
            Symbol("compressor"),
            Symbol("detector"),
            Symbol("filter"),
            Integer(1),
            Symbol("gain"),
            Symbol("value"),
            Float(100.0));
        instance.Output.WaitForResponse(requestId);

        Assert.Equal("error", Assert.Single(instance.Output.Messages).Selector);
    }

    [Fact]
    public void DetectorFilterGainWriteUpdatesTheFilterGainState()
    {
        using var application = new ManagedApplicationFixture();
        var instance = application.RegisterInstance();
        application.Send(
            instance,
            "observe_target",
            Symbol(instance.InstanceId.Value.ToString()),
            Integer(0));
        application.Send(instance, "set_instance_active", Integer(1));
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
        var change = messages.Last(message => message.Selector == "state_changed");
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
            message => message.Selector == "target_state_snapshot");
        var thresholdIndex = Enumerable.Range(0, (int)threshold.Atoms[5].Integer)
            .Single(index => threshold.Atoms[6 + index * 6].Symbol == "compressor.threshold");
        Assert.Equal(-97.0, threshold.Atoms[10 + thresholdIndex * 6].Float);
        Assert.Equal(0.0, threshold.Atoms[11 + thresholdIndex * 6].Float);
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
        application.Send(source, "set_instance_active", Integer(1));
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
            message => message.Selector == "target_state_snapshot" &&
                Enumerable.Range(0, (int)message.Atoms[5].Integer).Any(index =>
                    message.Atoms[6 + index * 6].Symbol == "compressor.threshold" &&
                    message.Atoms[10 + index * 6].Float == -97.0 &&
                    message.Atoms[11 + index * 6].Float == 0.0));
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
        application.Send(target, "set_instance_active", Integer(1));
        application.Send(
            localObserver,
            "observe_target",
            Symbol(target.InstanceId.Value.ToString()),
            Integer(1));
        application.Send(localObserver, "set_instance_active", Integer(1));
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
    public void MovingEqualizerMarkerPublishesAnUpdatedCurve()
    {
        using var application = new ManagedApplicationFixture();
        var instance = application.RegisterInstance();

        application.Send(
            instance,
            "observe_target",
            Symbol(instance.InstanceId.Value.ToString()),
            Integer(1));
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
            Symbol("filter"),
            Integer(1),
            Symbol("bypass"),
            Symbol("value"),
            Integer(0));
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
            Symbol("filter"),
            Integer(1),
            Symbol("gain"),
            Symbol("value"),
            Float(6.0));
        instance.Output.Clear();
        application.Send(instance, "set_instance_active", Integer(1));

        Assert.True(SpinWait.SpinUntil(
            () => instance.Output.Messages.Any(message =>
                message.Selector == "equalizer_curves"),
            TimeSpan.FromSeconds(1)));
        var initialCurve = instance.Output.Messages
            .Last(message => message.Selector == "equalizer_curves")
            .Atoms
            .Select(atom => atom.Float)
            .ToArray();
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
            Symbol("filter"),
            Integer(1),
            Symbol("frequency"),
            Symbol("value"),
            Float(3000.0));

        Assert.True(SpinWait.SpinUntil(
            () => instance.Output.Messages
                .Where(message => message.Selector == "equalizer_curves")
                .Select(message => message.Atoms.Select(atom => atom.Float).ToArray())
                .Any(curve => !curve.SequenceEqual(initialCurve)),
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
    public void FortyConnectedInstancesApplyCallbacklessInputGainGestureWithinLatencyBudget()
    {
        const int instanceCount = 40;
        const int writeCount = 120;
        var latencyBudget = TimeSpan.FromMilliseconds(50);
        using var application = new ManagedApplicationFixture();
        var instances = Enumerable.Range(0, instanceCount)
            .Select(_ => application.RegisterInstance())
            .ToArray();
        var editor = instances[0];

        application.Send(
            editor,
            "observe_target",
            Symbol(editor.InstanceId.Value.ToString()),
            Integer(7));
        application.Send(
            editor,
            "write",
            Symbol(editor.InstanceId.Value.ToString()),
            Symbol("0"),
            Integer(1),
            Symbol("entry"),
            Symbol("input_gain"),
            Symbol("gain"),
            Symbol("value"),
            Float(2.0));
        foreach (var instance in instances)
        {
            instance.Output.Clear();
        }

        var finalValue = 0.0;
        var startedAt = Stopwatch.StartNew();
        for (var index = 0; index < writeCount; index++)
        {
            finalValue = 1.0 + index % 24;
            application.Enqueue(
                editor,
                "write",
                Symbol(editor.InstanceId.Value.ToString()),
                Symbol("42"),
                Integer(1),
                Symbol("entry"),
                Symbol("input_gain"),
                Symbol("gain"),
                Symbol("value"),
                Float(finalValue));
        }
        Assert.True(SpinWait.SpinUntil(
            () => instances.All(instance =>
                instance.Dsp.Latest.Gain == (float)finalValue),
            TimeSpan.FromSeconds(5)),
            "The final gesture value did not reach every DSP snapshot.");
        startedAt.Stop();

        _output.WriteLine(
            $"40-instance input-gain gesture latency: {startedAt.Elapsed.TotalMilliseconds:F3} ms");
        Assert.All(
            instances,
            instance => Assert.Equal((float)finalValue, instance.Dsp.Latest.Gain));
        Assert.DoesNotContain(
            editor.Output.Messages,
            message => message.Selector == "action_done");
        Assert.True(
            startedAt.Elapsed <= latencyBudget,
            $"Final gesture value took {startedAt.Elapsed.TotalMilliseconds:F3} ms; " +
            $"budget is {latencyBudget.TotalMilliseconds:F0} ms.");
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
