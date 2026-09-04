using System.Collections.Generic;

using Consolidator.Managed.Core.State.Identifiers;
using Consolidator.Managed.Protocol.Messages;
using Consolidator.Managed.Protocol.Notifications;
using Consolidator.Managed.Protocol.Transport;
using Xunit;

namespace Consolidator.Managed.Tests.Transport;

public sealed class DspTelemetryPublisherTests
{
    [Fact]
    public void InactiveSourceDoesNotProduceATelemetryFrame()
    {
        var transport = new RecordingTransport();
        var publisher = new DspTelemetryPublisher(transport, _ => 0);

        publisher.PublishInputLevel(new InstanceId(7), 12, -18.0F, -6.0F);

        Assert.Empty(transport.Messages);
    }

    [Fact]
    public void ActiveSourceProducesOneVersionedLatestOnlyFrame()
    {
        var transport = new RecordingTransport();
        var publisher = new DspTelemetryPublisher(transport, _ => 3);

        publisher.PublishInputLevel(new InstanceId(7), 12, -18.0F, -6.0F);

        var message = Assert.Single(transport.Messages);
        Assert.Equal(new[] { 3UL }, message.TargetInstanceIds);
        Assert.Equal("dsp_telemetry", message.Selector);
        Assert.Equal(DeliverySemantics.LatestAnalysis, message.DeliverySemantics);
        Assert.Equal(1, message.Atoms[0].Integer);
        Assert.Equal(7, message.Atoms[1].Integer);
        Assert.Equal(12, message.Atoms[2].Integer);
        Assert.Equal(2, message.Atoms[3].Integer);
        Assert.Equal("input_gain.level", message.Atoms[4].Symbol);
        Assert.Equal(-18.0, message.Atoms[5].Float);
        Assert.Equal("input_gain.level_fast", message.Atoms[6].Symbol);
        Assert.Equal(-6.0, message.Atoms[7].Float);
    }

    private sealed class RecordingTransport : IPresentationTransport
    {
        public List<ProtocolOutput> Messages { get; } = [];

        public void Send(ProtocolOutput message)
        {
            Messages.Add(message);
        }

        public void SetActive(ulong instanceId, bool active)
        {
        }

        public void Unregister(ulong instanceId)
        {
        }
    }
}
