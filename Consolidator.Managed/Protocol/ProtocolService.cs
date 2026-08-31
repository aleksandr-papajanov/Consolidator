using System.Collections.Concurrent;
using System.Diagnostics;
using System.Threading.Channels;

using Consolidator.Managed.Core.Commands.Definitions;
using Consolidator.Managed.Core.Services;
using Consolidator.Managed.Protocol.Decoding;
using Consolidator.Managed.Protocol.Dispatch;
using Consolidator.Managed.Protocol.Encoding;
using Consolidator.Managed.Protocol.Messages;
using Consolidator.Managed.Protocol.Transport;

namespace Consolidator.Managed.Protocol;

internal sealed class ProtocolService : IDisposable
{
    private const int QueueCapacity = 256;
    private readonly CommandDecoder _decoder;
    private readonly CommandEndpointRegistry _endpoints;
    private readonly IProtocolTransport _transport;
    private readonly Channel<QueuedCommand> _commands = Channel.CreateBounded<QueuedCommand>(
        new BoundedChannelOptions(QueueCapacity)
        {
            FullMode = BoundedChannelFullMode.Wait,
            SingleReader = true,
            SingleWriter = false
        });
    private readonly ConcurrentDictionary<ulong, byte> _cancelledInstances = new();
    private readonly Dictionary<CoalesceKey, QueuedCommand> _coalesced = new();
    private readonly object _coalesceLock = new();
    private readonly CancellationTokenSource _cancellation = new();
    private readonly Task _worker;

    public ProtocolService(
        CommandDecoder decoder,
        CommandEndpointRegistry endpoints,
        IProtocolTransport transport)
    {
        ArgumentNullException.ThrowIfNull(decoder);
        ArgumentNullException.ThrowIfNull(endpoints);
        ArgumentNullException.ThrowIfNull(transport);

        _decoder = decoder;
        _endpoints = endpoints;
        _transport = transport;
        _worker = Task.Run(ProcessCommandsAsync);
    }

    public void Receive(
        ProtocolInput message)
    {
        ArgumentNullException.ThrowIfNull(message);
        DecodedCommand decoded;
        try
        {
            decoded = _decoder.Decode(message);
        }
        catch (Exception exception)
        {
            SendError(message, exception);
            return;
        }

        if (_cancelledInstances.ContainsKey(message.SourceInstanceId))
        {
            return;
        }

        var queued = new QueuedCommand(decoded, GetCoalesceKey(decoded));
        if (queued.Key is { } key)
        {
            lock (_coalesceLock)
            {
                if (_coalesced.ContainsKey(key))
                {
                    _coalesced[key] = queued;
                    return;
                }

                _coalesced.Add(key, queued);
                if (_commands.Writer.TryWrite(queued))
                {
                    return;
                }

                _coalesced.Remove(key);
            }

            SendError(message, new ProtocolOverloadedException());
            return;
        }

        if (!_commands.Writer.TryWrite(queued))
        {
            SendError(message, new ProtocolOverloadedException());
        }
    }

    public void CancelInstance(ulong instanceId)
    {
        _cancelledInstances[instanceId] = 1;
    }

    public T ExecuteControlBarrier<T>(Func<T> action)
    {
        ArgumentNullException.ThrowIfNull(action);
        var completion = new TaskCompletionSource<object?>(
            TaskCreationOptions.RunContinuationsAsynchronously);
        _commands.Writer.WriteAsync(
            new QueuedCommand(null, null, () => action()!, completion),
            _cancellation.Token).AsTask().GetAwaiter().GetResult();
        return (T)completion.Task.GetAwaiter().GetResult()!;
    }

    public void Dispose()
    {
        _commands.Writer.TryComplete();
        _cancellation.Cancel();
        try
        {
            _worker.GetAwaiter().GetResult();
        }
        catch (OperationCanceledException)
        {
        }

        while (_commands.Reader.TryRead(out var queued))
        {
            queued.Completion?.TrySetException(
                new ObjectDisposedException(nameof(ProtocolService)));
        }

        _cancellation.Dispose();
    }

    private async Task ProcessCommandsAsync()
    {
        try
        {
            await foreach (var queued in _commands.Reader.ReadAllAsync(_cancellation.Token))
            {
                if (queued.Barrier is not null)
                {
                    try
                    {
                        queued.Completion!.SetResult(queued.Barrier());
                    }
                    catch (Exception exception)
                    {
                        queued.Completion!.SetException(exception);
                    }

                    continue;
                }

                var command = queued.Command
                    ?? throw new InvalidOperationException("A command queue entry was empty.");
                if (queued.Key is { } key)
                {
                    lock (_coalesceLock)
                    {
                        if (_coalesced.Remove(key, out var latest))
                        {
                            command = latest.Command
                                ?? throw new InvalidOperationException("A coalesced command was empty.");
                        }
                    }
                }

                if (_cancelledInstances.ContainsKey(command.SourceInstanceId))
                {
                    continue;
                }

                var startedAt = Stopwatch.GetTimestamp();
                try
                {
                    var responses = await _endpoints.ExecuteAsync(
                        command,
                        _cancellation.Token);
                    foreach (var response in responses)
                    {
                        _transport.Send(response);
                    }
                }
                catch (Exception exception)
                {
                    _transport.Send(ProtocolErrorEncoder.Encode(
                        command.SourceInstanceId,
                        command.RequestId,
                        exception));
                }
                finally
                {
                    RuntimeMetrics.Shared.RecordControlOperation(
                        Stopwatch.GetTimestamp() - startedAt);
                }
            }
        }
        catch (OperationCanceledException)
        {
        }
    }

    private void SendError(ProtocolInput message, Exception exception)
    {
        _transport.Send(ProtocolErrorEncoder.Encode(
            message.SourceInstanceId,
            message.Atoms.Count > 2 &&
            message.Atoms[2].Type is AtomType.Symbol &&
            ulong.TryParse(message.Atoms[2].Symbol, out var requestId)
                ? requestId
                : 0,
            exception));
    }

    private static CoalesceKey? GetCoalesceKey(DecodedCommand decoded)
    {
        if (decoded.Command is not WriteStateCommand
            {
                Entries.Count: > 0,
                TransactionId: not 0
            } write)
        {
            return null;
        }

        return new CoalesceKey(
            decoded.SourceInstanceId,
            write.TargetInstanceId?.Value ?? decoded.SourceInstanceId,
            string.Join("|", write.Entries.Select(entry => entry.Path)),
            write.TransactionId);
    }

    private sealed record QueuedCommand(
        DecodedCommand? Command,
        CoalesceKey? Key,
        Func<object?>? Barrier = null,
        TaskCompletionSource<object?>? Completion = null);

    private readonly record struct CoalesceKey(
        ulong SourceInstanceId,
        ulong TargetInstanceId,
        string PathShape,
        ulong TransactionId);
}
