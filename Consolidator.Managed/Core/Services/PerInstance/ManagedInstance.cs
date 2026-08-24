using Consolidator.Managed.Core.Dsp;
using Consolidator.Managed.Core.State;
using Consolidator.Managed.Core.State.Models;

namespace Consolidator.Managed.Core.Services.PerInstance;

internal sealed class ManagedInstance : IDisposable
{
    private readonly IDspStatePublisher _dspPublisher;
    private readonly SemaphoreSlim _commandGate = new(1, 1);
    private bool _disposed;

    internal ManagedInstance(
        InstanceId instanceId,
        ManagedState state,
        IDspStatePublisher dspPublisher)
    {
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(dspPublisher);

        InstanceId = instanceId;
        State = state;
        _dspPublisher = dspPublisher;
    }

    internal InstanceId InstanceId { get; }

    internal ManagedState State { get; }

    internal void PublishDspState()
    {
        _dspPublisher.Publish(State.Runtime.Snapshot);
    }

    internal async ValueTask<TResult> ExecuteAsync<TResult>(
        Func<ManagedState, ValueTask<TResult>> operation,
        CancellationToken cancellationToken)
    {
        ArgumentNullException.ThrowIfNull(operation);

        await _commandGate.WaitAsync(cancellationToken);
        try
        {
            return await operation(State);
        }
        finally
        {
            _commandGate.Release();
        }
    }

    public void Dispose()
    {
        if (_disposed)
        {
            return;
        }

        _commandGate.Wait();
        try
        {
            _dspPublisher.Stop();
            _disposed = true;
        }
        finally
        {
            _commandGate.Release();
            _commandGate.Dispose();
        }
    }
}
