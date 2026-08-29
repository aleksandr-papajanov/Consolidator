using Consolidator.Managed.Core.Services.Abstractions;
using Consolidator.Managed.Core.State.Models;

namespace Consolidator.Managed.Core.State.Observers;

internal sealed class ProcessorActivityObserver
{
    private const float GainEpsilon = 0.0001F;
    private readonly InstanceId _instanceId;
    private readonly DspState _dsp;
    private readonly IProcessorStatusSink _sink;
    private readonly ProcessorStatus[] _statuses;

    public ProcessorActivityObserver(
        InstanceId instanceId,
        DspState dsp,
        IProcessorStatusSink sink)
    {
        _instanceId = instanceId;
        _dsp = dsp;
        _sink = sink;
        _statuses = ProcessorIds.All.Select(CreateStatus).ToArray();
    }

    public void Refresh()
    {
        for (var index = 0; index < _statuses.Length; index++)
        {
            var current = CreateStatus(_statuses[index].ProcessorId);
            if (_statuses[index] == current)
            {
                continue;
            }

            _statuses[index] = current;
            _sink.ProcessorStatusChanged(_instanceId, current);
        }
    }

    public IReadOnlyList<ProcessorStatus> Snapshot() => _statuses.ToArray();

    private ProcessorStatus CreateStatus(ProcessorId processorId) => processorId switch
    {
        ProcessorId.Input => new(
            processorId,
            !_dsp.InputGain.Bypass.Value &&
                MathF.Abs(_dsp.InputGain.GainDb.Value - 1.0F) > GainEpsilon,
            _dsp.InputGain.Bypass.Value,
            false),
        ProcessorId.Saturator => new(
            processorId,
            !_dsp.Saturator.Bypass.Value &&
                (MathF.Abs(_dsp.Saturator.Drive.Value) > GainEpsilon ||
                    MathF.Abs(_dsp.Saturator.OutputDb.Value) > GainEpsilon ||
                    MathF.Abs(_dsp.Saturator.Mix.Value - 1.0F) > GainEpsilon ||
                    MathF.Abs(_dsp.Saturator.DetectorAmount.Value - 1.0F) > GainEpsilon),
            _dsp.Saturator.Bypass.Value,
            _dsp.Saturator.Solo.Value),
        ProcessorId.Compressor => new(
            processorId,
            !_dsp.Compressor.Bypass.Value &&
                (MathF.Abs(_dsp.Compressor.ThresholdDb.Value + 24.0F) > GainEpsilon ||
                    MathF.Abs(_dsp.Compressor.Ratio.Value - 4.0F) > GainEpsilon ||
                    MathF.Abs(_dsp.Compressor.AttackMs.Value - 10.0F) > GainEpsilon ||
                    MathF.Abs(_dsp.Compressor.ReleaseMs.Value - 100.0F) > GainEpsilon ||
                    MathF.Abs(_dsp.Compressor.OutputDb.Value) > GainEpsilon ||
                    MathF.Abs(_dsp.Compressor.Mix.Value - 1.0F) > GainEpsilon),
            _dsp.Compressor.Bypass.Value,
            _dsp.Compressor.Solo.Value),
        ProcessorId.Equalizer => new(
            processorId,
            !_dsp.Equalizer.Bypass.Value &&
                _dsp.EqualizerBanks.Any(bank => bank.EffectActive),
            _dsp.Equalizer.Bypass.Value,
            _dsp.Equalizer.Solo.Value),
        ProcessorId.Output => new(
            processorId,
            !_dsp.OutputGain.Bypass.Value &&
                MathF.Abs(_dsp.OutputGain.GainDb.Value - 1.0F) > GainEpsilon,
            _dsp.OutputGain.Bypass.Value,
            false),
        _ => throw new ArgumentOutOfRangeException(nameof(processorId))
    };
}
