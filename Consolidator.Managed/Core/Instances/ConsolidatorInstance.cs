using Consolidator.Managed.Core.Abstractions;
using Consolidator.Managed.Core.Dsp;
using Consolidator.Managed.Core.State;
using Consolidator.Managed.Protocol;

namespace Consolidator.Managed.Core.Instances;

public sealed class ConsolidatorInstance
{
    private readonly object _lifecycleLock = new();
    private readonly IInstanceOutput _output;
    private readonly DspStateCompiler _dspCompiler;
    private readonly IDspStatePublisher _dspPublisher;
    private readonly DspState _state;
    private readonly InstanceStateStore _stateStore;
    private bool _active = true;

    public ConsolidatorInstance(
        ulong id,
        IInstanceOutput output,
        IDspStatePublisher dspPublisher,
        StateHistory history,
        DspStateCompiler dspCompiler,
        DspState state,
        InstanceStateBuilder stateBuilder)
    {
        Id = id;
        _output = output;
        _dspCompiler = dspCompiler;
        _dspPublisher = dspPublisher;
        _state = state;
        _stateStore = stateBuilder.Build(history, state);
        PublishDspState();
    }

    public ulong Id { get; }

    public InstanceStateStore StateStore => _stateStore;

    public bool TrySend(
        string selector,
        ReadOnlySpan<Atom> atoms)
    {
        lock (_lifecycleLock)
        {
            if (!_active)
            {
                return false;
            }

            _output.Send(selector, atoms);
            return true;
        }
    }

    public void Prepare(
        double sampleRate,
        nuint maximumFrameCount)
    {
        lock (_lifecycleLock)
        {
            if (!_active)
            {
                return;
            }

            // Later: update the DSP compilation context.
        }
    }

    public unsafe void ReceiveAudio(
        double* mainLeft,
        double* mainRight,
        double* referenceLeft,
        double* referenceRight,
        nuint frameCount)
    {
        if (!Volatile.Read(ref _active))
        {
            return;
        }

        // Analyzer input will consume these buffers without Coordinator lookup.
    }

    public void Stop()
    {
        lock (_lifecycleLock)
        {
            if (!_active)
            {
                return;
            }

            _active = false;
            _stateStore.Dispose();
            _dspPublisher.Stop();
        }
    }

    public void PublishDspState()
    {
        lock (_lifecycleLock)
        {
            if (!_active)
            {
                return;
            }

            var snapshot = _dspCompiler.Compile(_state);
            _dspPublisher.Publish(snapshot);
        }
    }

}