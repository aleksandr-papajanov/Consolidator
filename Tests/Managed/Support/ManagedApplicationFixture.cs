using System;
using System.Collections.Generic;

using Consolidator.Managed.Core.Dsp;
using Consolidator.Managed.Core.Services.Abstractions;
using Consolidator.Managed.Core.State;
using Consolidator.Managed.Native;
using Consolidator.Managed.Protocol;
using Consolidator.Managed.Protocol.Messages;
using Consolidator.Managed.Protocol.Transport;
using Consolidator.Managed.Services;
using Microsoft.Extensions.DependencyInjection;
using Xunit;

namespace Consolidator.Managed.Tests.Support;

internal sealed class ManagedApplicationFixture : IDisposable
{
    private readonly ServiceProvider _provider = ManagedServices.CreateProvider();
    private readonly IInstanceLifecycleService _lifecycle;
    private readonly IProtocolOutputRegistry _outputs;
    private readonly ProtocolService _protocol;
    private ulong _nextRequestId;

    public ManagedApplicationFixture()
    {
        _lifecycle = _provider.GetRequiredService<IInstanceLifecycleService>();
        _outputs = _provider.GetRequiredService<IProtocolOutputRegistry>();
        _protocol = _provider.GetRequiredService<ProtocolService>();
    }

    public TestInstance RegisterInstance()
    {
        var publisher = new RecordingDspPublisher();
        var instanceId = _lifecycle.RegisterInstance(publisher);
        var output = new RecordingOutputCallback();
        _outputs.Register(instanceId.Value, output);
        return new TestInstance(instanceId, publisher, output);
    }

    public ulong Send(
        TestInstance source,
        string selector,
        params Atom[] body)
    {
        var requestId = ++_nextRequestId;
        var atoms = new List<Atom>
        {
            Integer(1),
            Symbol("test"),
            Symbol(requestId.ToString())
        };
        atoms.AddRange(body);
        _protocol.Receive(new ProtocolInput(
            source.InstanceId.Value,
            selector,
            atoms));
        return requestId;
    }

    public void UnregisterInstance(TestInstance instance)
    {
        _outputs.Unregister(instance.InstanceId.Value);
        _lifecycle.UnregisterInstance(instance.InstanceId);
    }

    public void Dispose()
    {
        _provider.Dispose();
    }

    public static Atom Integer(long value) =>
        new(AtomType.Integer, value, 0, null);

    public static Atom Float(double value) =>
        new(AtomType.Float, 0, value, null);

    public static Atom Symbol(string value) =>
        new(AtomType.Symbol, 0, 0, value);

    internal sealed record TestInstance(
        InstanceId InstanceId,
        RecordingDspPublisher Dsp,
        RecordingOutputCallback Output);

    internal sealed class RecordingDspPublisher : IDspStatePublisher
    {
        public DspSnapshot Latest { get; private set; }

        public int PublishCount { get; private set; }

        public bool Stopped { get; private set; }

        public void Publish(in DspSnapshot snapshot)
        {
            Latest = snapshot;
            PublishCount++;
        }

        public void Stop()
        {
            Stopped = true;
        }
    }

    internal sealed class RecordingOutputCallback : IProtocolOutputCallback
    {
        public List<ProtocolOutput> Messages { get; } = new();

        public void Send(ProtocolOutput message)
        {
            Messages.Add(message);
        }

        public void Clear()
        {
            Messages.Clear();
        }

        public ProtocolOutput Single(string selector) =>
            Assert.Single(Messages, message => message.Selector == selector);
    }
}
