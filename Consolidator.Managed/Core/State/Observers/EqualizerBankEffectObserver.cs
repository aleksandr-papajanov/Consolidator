using Consolidator.Managed.Core.Services.Abstractions;
using Consolidator.Managed.Core.Settings;
using Consolidator.Managed.State;
using Consolidator.Managed.State.Observers;

namespace Consolidator.Managed.Core.State.Observers;

internal sealed class EqualizerBankEffectObserver
{
    private const float GainEpsilon = 0.0001F;

    private readonly InstanceId _instanceId;
    private readonly int _bankId;
    private readonly IBankEffectStatusSink _sink;
    private readonly bool[] _filterBypasses = new bool[DspConstants.EqualizerFilterCount];
    private readonly float[] _filterGains = new float[DspConstants.EqualizerFilterCount];
    private bool _bankBypass;
    private bool _effectActive;

    public EqualizerBankEffectObserver(
        InstanceId instanceId,
        int bankId,
        IBankEffectStatusSink sink)
    {
        _instanceId = instanceId;
        _bankId = bankId;
        _sink = sink;
    }

    public IStateValueObserver<bool> ObserveBankBypass() =>
        new ValueObserver<bool>(this, (observer, value) => observer._bankBypass = value);

    public IStateValueObserver<bool> ObserveFilterBypass(int filterId) =>
        new ValueObserver<bool>(
            this,
            (observer, value) => observer._filterBypasses[filterId] = value);

    public IStateValueObserver<float> ObserveFilterGain(int filterId) =>
        new ValueObserver<float>(
            this,
            (observer, value) => observer._filterGains[filterId] = value);

    public void Initialize(
        bool bankBypass,
        IReadOnlyList<bool> filterBypasses,
        IReadOnlyList<float> filterGains)
    {
        _bankBypass = bankBypass;
        for (var index = 0; index < _filterBypasses.Length; index++)
        {
            _filterBypasses[index] = filterBypasses[index];
            _filterGains[index] = filterGains[index];
        }

        _effectActive = CalculateEffectActive();
    }

    private void ValueChanged()
    {
        var effectActive = CalculateEffectActive();
        if (_effectActive == effectActive)
        {
            return;
        }

        _effectActive = effectActive;
        _sink.BankEffectStatusChanged(_instanceId, _bankId, effectActive);
    }

    private bool CalculateEffectActive()
    {
        if (_bankBypass)
        {
            return false;
        }

        for (var index = 0; index < _filterBypasses.Length; index++)
        {
            if (!_filterBypasses[index] &&
                MathF.Abs(_filterGains[index]) > GainEpsilon)
            {
                return true;
            }
        }

        return false;
    }

    private sealed class ValueObserver<TValue> : IStateValueObserver<TValue>
    {
        private readonly EqualizerBankEffectObserver _owner;
        private readonly Action<EqualizerBankEffectObserver, TValue> _assign;

        public ValueObserver(
            EqualizerBankEffectObserver owner,
            Action<EqualizerBankEffectObserver, TValue> assign)
        {
            _owner = owner;
            _assign = assign;
        }

        public void Attach(StateValue<TValue> value)
        {
            _assign(_owner, value.Value);
        }

        public void ValueChanged(
            StateValue<TValue> value,
            TValue previousValue,
            TValue currentValue)
        {
            _assign(_owner, currentValue);
            _owner.ValueChanged();
        }

        public void Detach(StateValue<TValue> value)
        {
        }
    }
}
