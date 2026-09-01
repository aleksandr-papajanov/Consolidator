using System.Collections.Generic;
using Consolidator.Managed.Protocol.Messages;
using Consolidator.Managed.Protocol.Transport;
using Xunit;

namespace Consolidator.Managed.Tests.Transport;

public sealed class PresentationOutputGateTests
{
    [Fact]
    public void InactiveRecipientIsDiscardedWithoutActivationReplay()
    {
        var output = new RecordingTransport();
        var gate = new PresentationOutputGate(output);

        gate.Send(StateChange(7, "compressor.attack", -18));
        gate.Send(StateChange(7, "compressor.attack", 15));
        gate.SetActive(7, true);

        Assert.Empty(output.Messages);
    }

    [Fact]
    public void ActiveRecipientReceivesStateChangeImmediately()
    {
        var output = new RecordingTransport();
        var gate = new PresentationOutputGate(output);
        gate.SetActive(7, true);

        gate.Send(StateChange(7, "compressor.attack", -18));

        var message = Assert.Single(output.Messages);
        Assert.Equal("state_changed", message.Selector);
        Assert.Equal(-18, message.Atoms[2].Float);
    }

    [Fact]
    public void LosslessOutputIsDeliveredToInactiveRecipient()
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
    public void UnregisterStopsActivePresentationDelivery()
    {
        var output = new RecordingTransport();
        var gate = new PresentationOutputGate(output);
        gate.SetActive(7, true);
        gate.Send(StateChange(7, "compressor.attack", -18));
        output.Messages.Clear();

        gate.Unregister(7);
        gate.Send(StateChange(7, "compressor.attack", -12));

        Assert.Empty(output.Messages);
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
            DeliverySemantics.ActivePresentation);

    private sealed class RecordingTransport : IProtocolTransport
    {
        public List<ProtocolOutput> Messages { get; } = [];

        public void Send(ProtocolOutput message)
        {
            Messages.Add(message);
        }
    }
}
