using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;

using Consolidator.Managed.Core.Dsp;
using Consolidator.Managed.Core.Services.Abstractions;
using Consolidator.Managed.Core.State.Identifiers;
using Consolidator.Managed.Protocol;
using Consolidator.Managed.Protocol.Messages;
using Consolidator.Managed.Protocol.Transport;
using Consolidator.Managed.Composition;
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
        var requestId = Enqueue(source, selector, body);
        source.Output.WaitForResponse(requestId);
        return requestId;
    }

    public ulong Enqueue(
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
        atoms.AddRange(NormalizeLegacyWrite(body, selector));
        _protocol.Receive(new ProtocolInput(
            source.InstanceId.Value,
            selector,
            atoms));
        return requestId;
    }

    private static IEnumerable<Atom> NormalizeLegacyWrite(
        IReadOnlyList<Atom> body,
        string selector)
    {
        if (selector != "write")
        {
            return body;
        }

        var normalized = new List<Atom>(body.Count + 2);
        for (var index = 0; index < body.Count; index++)
        {
            var atom = body[index];
            normalized.Add(atom);
            if (atom.Type != AtomType.Symbol || atom.Symbol != "value" ||
                index + 1 >= body.Count)
            {
                continue;
            }

            var value = body[++index];
            normalized.Add(value);
            if (index + 1 >= body.Count ||
                body[index + 1].Type != AtomType.Symbol ||
                body[index + 1].Symbol is not ("copy" or "delta"))
            {
                normalized.Add(Symbol(value.Type == AtomType.Float ? "delta" : "copy"));
            }
        }

        return normalized;
    }

    public void UnregisterInstance(TestInstance instance)
    {
        _outputs.Unregister(instance.InstanceId.Value);
        _lifecycle.UnregisterInstance(instance.InstanceId);
    }

    public TService GetRequiredService<TService>()
        where TService : notnull
    {
        return _provider.GetRequiredService<TService>();
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
        private static readonly HashSet<string> TerminalSelectors =
        [
            "action_done",
            "error",
            "initialized",
            "registry_done",
            "state_done",
            "target_state_snapshot"
        ];
        private readonly List<ProtocolOutput> _messages = new();
        private readonly object _lock = new();

        public IReadOnlyList<ProtocolOutput> Messages
        {
            get
            {
                lock (_lock)
                {
                    return _messages.ToArray();
                }
            }
        }

        public void Send(ProtocolOutput message)
        {
            lock (_lock)
            {
                _messages.Add(message);
            }
        }

        public void Clear()
        {
            lock (_lock)
            {
                _messages.Clear();
            }
        }

        public ProtocolOutput Single(string selector)
        {
            lock (_lock)
            {
                return Assert.Single(
                    _messages,
                    message => message.Selector == selector);
            }
        }

        public void WaitForResponse(ulong requestId)
        {
            Assert.True(SpinWait.SpinUntil(
                () => HasResponse(requestId),
                TimeSpan.FromSeconds(5)),
                $"No terminal response was received for request {requestId}.");
        }

        private bool HasResponse(ulong requestId)
        {
            var expectedRequestId = requestId.ToString();
            lock (_lock)
            {
                return _messages.Any(message =>
                    TerminalSelectors.Contains(message.Selector) &&
                    message.Atoms.Count > 2 &&
                    message.Atoms[2].Type is AtomType.Symbol &&
                    message.Atoms[2].Symbol == expectedRequestId);
            }
        }
    }
}
