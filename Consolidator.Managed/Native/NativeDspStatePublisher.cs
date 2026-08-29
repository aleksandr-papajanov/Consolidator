using Consolidator.Managed.Core.Dsp;

namespace Consolidator.Managed.Native;

public unsafe sealed class NativeDspStatePublisher : IDspStatePublisher
{
    private readonly object _publishLock = new();
    private SharedDspExchange* _exchange;

    public NativeDspStatePublisher(SharedDspExchange* exchange)
    {
        if (exchange == null)
        {
            throw new ArgumentNullException(nameof(exchange));
        }

        _exchange = exchange;
    }

    public void Publish(in DspSnapshot snapshot)
    {
        lock (_publishLock)
        {
            if (_exchange == null)
            {
                return;
            }

            PublishCore(snapshot);
        }
    }

    public void Stop()
    {
        lock (_publishLock)
        {
            _exchange = null;
        }
    }

    private void PublishCore(in DspSnapshot snapshot)
    {
        var publishedIndex =
            Volatile.Read(ref _exchange->PublishedIndex);
        var consumerIndex =
            Volatile.Read(ref _exchange->ConsumerIndex);
        var writeIndex = FindWritableIndex(publishedIndex, consumerIndex);

        switch (writeIndex)
        {
            case 0:
                _exchange->Snapshot0 = snapshot;
                break;
            case 1:
                _exchange->Snapshot1 = snapshot;
                break;
            default:
                _exchange->Snapshot2 = snapshot;
                break;
        }

        Volatile.Write(
            ref _exchange->PublishedIndex,
            writeIndex);
    }

    private static uint FindWritableIndex(
        uint publishedIndex,
        uint consumerIndex)
    {
        for (uint index = 0; index < 3; ++index)
        {
            if (index != publishedIndex &&
                index != consumerIndex)
            {
                return index;
            }
        }

        throw new InvalidOperationException(
            "No writable DSP snapshot slot is available.");
    }
}



