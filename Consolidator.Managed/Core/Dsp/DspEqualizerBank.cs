namespace Consolidator.Managed.Core.Dsp;

public sealed class DspEqualizerBank
{
    private readonly DspRuntimeState _runtime;
    private readonly int _index;

    internal DspEqualizerBank(DspRuntimeState runtime, int index)
    {
        _runtime = runtime;
        _index = index;
    }

    public bool Active
    {
        get => _runtime.IsEqualizerBankActive(_index);
        set => _runtime.SetEqualizerBankActive(_index, value);
    }

    public ref FilterSnapshot this[int filterIndex] =>
        ref _runtime[_index, filterIndex];
}