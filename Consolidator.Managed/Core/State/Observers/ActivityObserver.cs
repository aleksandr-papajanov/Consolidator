using Consolidator.Managed.Core.Services.Abstractions;
using Consolidator.Managed.Core.State.Models;
using Consolidator.Managed.Core.Settings;
using Consolidator.Managed.State;
using Consolidator.Managed.State.Observers;

namespace Consolidator.Managed.Core.State.Observers;

internal sealed class ActivityObserver
{
    public const float GainEpsilon = 0.5F;

    private readonly InstanceId _instanceId;
    private DspState _dsp = null!;
    private readonly IActivityStatusSink _sink;
    private readonly bool[] _filterBypasses = new bool[DspConstants.EqualizerFilterCount * DspConstants.BankCount];
    private readonly float[] _filterGains = new float[DspConstants.EqualizerFilterCount * DspConstants.BankCount];
    private readonly bool[] _bankBypasses = new bool[DspConstants.BankCount];
    private readonly bool[] _bankActivities = new bool[DspConstants.BankCount];
    private readonly ProcessorStatus[] _statuses;

    public ActivityObserver(
        InstanceId instanceId,
        IActivityStatusSink sink)
    {
        _instanceId = instanceId;
        _sink = sink;
        _statuses = ProcessorIds.All
            .Select(processorId => new ProcessorStatus(processorId, false, false, false))
            .ToArray();
    }

    public void Initialize(DspState dsp)
    {
        _dsp = dsp;
        for (var bankId = 0; bankId < DspConstants.BankCount; bankId++)
        {
            RefreshBank(bankId, false);
        }

        for (var index = 0; index < _statuses.Length; index++)
        {
            _statuses[index] = CreateStatus(_statuses[index].ProcessorId);
        }
    }

    public IStateValueObserver<bool> ObserveBankBypass(int bankId) =>
        new ValueObserver<bool>(this, (observer, value) => observer.SetBankBypass(bankId, value));

    public IStateValueObserver<bool> ObserveFilterBypass(int bankId, int filterId) =>
        new ValueObserver<bool>(this, (observer, value) => observer.SetFilterBypass(bankId, filterId, value));

    public IStateValueObserver<float> ObserveFilterGain(int bankId, int filterId) =>
        new ValueObserver<float>(this, (observer, value) => observer.SetFilterGain(bankId, filterId, value));

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
            _sink.ProcessorActivityChanged(_instanceId, current);
        }
    }

    public IReadOnlyList<ProcessorStatus> Snapshot() => _statuses.ToArray();

    public bool BankActivity(int bankId) => _bankActivities[bankId];

    private void SetBankBypass(int bankId, bool value)
    {
        _bankBypasses[bankId] = value;
        RefreshBank(bankId, true);
    }

    private void SetFilterBypass(int bankId, int filterId, bool value)
    {
        _filterBypasses[FilterIndex(bankId, filterId)] = value;
        RefreshBank(bankId, true);
    }

    private void SetFilterGain(int bankId, int filterId, float value)
    {
        _filterGains[FilterIndex(bankId, filterId)] = value;
        RefreshBank(bankId, true);
    }

    private void RefreshBank(int bankId, bool publish)
    {
        var active = !_bankBypasses[bankId];
        if (active)
        {
            active = Enumerable.Range(0, DspConstants.EqualizerFilterCount)
                .Any(filterId => !_filterBypasses[FilterIndex(bankId, filterId)] &&
                    MathF.Abs(_filterGains[FilterIndex(bankId, filterId)]) > GainEpsilon);
        }

        if (_bankActivities[bankId] == active)
        {
            return;
        }

        _bankActivities[bankId] = active;
        if (publish)
        {
            _sink.BankActivityChanged(_instanceId, bankId, active);
        }
    }

    private static int FilterIndex(int bankId, int filterId) =>
        bankId * DspConstants.EqualizerFilterCount + filterId;

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
            !_dsp.Equalizer.Bypass.Value && _bankActivities.Any(active => active),
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

    private sealed class ValueObserver<TValue> : IStateValueObserver<TValue>
    {
        private readonly ActivityObserver _owner;
        private readonly Action<ActivityObserver, TValue> _assign;

        public ValueObserver(ActivityObserver owner, Action<ActivityObserver, TValue> assign)
        {
            _owner = owner;
            _assign = assign;
        }

        public void Attach(StateValue<TValue> value) => _assign(_owner, value.Value);

        public void ValueChanged(StateValue<TValue> value, TValue previousValue, TValue currentValue) =>
            _assign(_owner, currentValue);

        public void Detach(StateValue<TValue> value)
        {
        }
    }
}
