using System.Collections.Generic;
using System.Linq;
using Consolidator.Managed.Protocol.Messages;
using Consolidator.Managed.Protocol.Transport;
using Xunit;

namespace Consolidator.Managed.Tests.Transport;

public sealed class PresentationOutputGateTests
{
    [Fact]
    public void InactiveRecipientCoalescesEachPathIntoOneActivationBatch()
    {
        var output = new RecordingTransport();
        var gate = new PresentationOutputGate(output);

        gate.Send(StateChange(7, "compressor.threshold", -24));
        gate.Send(StateChange(7, "compressor.threshold", -18));
        gate.Send(StateChange(7, "compressor.attack", 15));

        Assert.Empty(output.Messages);

        gate.SetActive(7, true);

        Assert.Equal(
            ["state_batch_begin", "state_batch_entry", "state_batch_entry", "state_batch_done"],
            output.Messages.Select(message => message.Selector));
        Assert.Equal("compressor.attack", output.Messages[1].Atoms[2].Symbol);
        Assert.Equal("compressor.threshold", output.Messages[2].Atoms[2].Symbol);
        Assert.Equal(-18, output.Messages[2].Atoms[3].Float);
    }

    [Fact]
    public void ActiveRecipientReceivesStateChangeImmediately()
    {
        var output = new RecordingTransport();
        var gate = new PresentationOutputGate(output);
        gate.SetActive(7, true);

        gate.Send(StateChange(7, "compressor.threshold", -18));

        var message = Assert.Single(output.Messages);
        Assert.Equal("state_changed", message.Selector);
        Assert.Equal(-18, message.Atoms[2].Float);
    }

    [Fact]
    public void LosslessOutputIsNeverDelayedForInactiveRecipient()
    {
        var output = new RecordingTransport();
        var gate = new PresentationOutputGate(output);

        gate.Send(new ProtocolOutput(
            [7],
            "action_done",
            [new Atom(AtomType.Integer, 1, 0, null)]));

        Assert.Equal("action_done", Assert.Single(output.Messages).Selector);
    }

    [Fact]
    public void TargetChangeAndUnregisterDiscardPendingState()
    {
        var output = new RecordingTransport();
        var gate = new PresentationOutputGate(output);

        gate.Send(StateChange(7, "compressor.threshold", -18));
        gate.SetObservedTarget(7, 9, 2);
        gate.SetActive(7, true);
        Assert.DoesNotContain(output.Messages, message => message.Selector == "state_batch_begin");

        gate.SetActive(7, false);
        gate.Send(StateChange(7, "compressor.threshold", -12));
        gate.Unregister(7);
        gate.SetActive(7, true);

        Assert.DoesNotContain(output.Messages, message => message.Selector == "state_batch_begin");
    }

    private static ProtocolOutput StateChange(ulong recipient, string path, double value) =>
        new(
            [recipient],
            "state_changed",
            [
                new Atom(AtomType.Integer, 1, 0, null),
                new Atom(AtomType.Symbol, 0, 0, path),
                new Atom(AtomType.Float, 0, value, null)
            ],
            DeliverySemantics.CoalescedPresentation,
            path);

    private sealed class RecordingTransport : IProtocolTransport
    {
        public List<ProtocolOutput> Messages { get; } = [];

        public void Send(ProtocolOutput message)
        {
            Messages.Add(message);
        }
    }
}
