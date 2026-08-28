using Consolidator.Managed.Core.Dsp;
using Consolidator.Managed.Core.Services.Abstractions;
using Consolidator.Managed.Core.Settings;
using Consolidator.Managed.State;

namespace Consolidator.Managed.Core.State.Models;

public sealed class DspState
{
    public DspState(
        InstanceId instanceId,
        StateValueFactory values,
        DspRuntimeState runtime,
        IBankEffectStatusSink effectStatusSink)
    {
        ArgumentNullException.ThrowIfNull(values);
        ArgumentNullException.ThrowIfNull(runtime);

        var dspPath = new StatePath([StateNodeIds.Dsp]);
        InputGain = new GainState(
            instanceId,
            dspPath.Append(StateNodeIds.InputGain),
            values,
            runtime.Gain,
            value => runtime.Gain = value,
            value =>
            {
                runtime.InputGainBypass = value;
                runtime.InputGainActive = !value;
            });
        Saturator = new SaturatorState(
            instanceId,
            dspPath.Append(StateNodeIds.Saturator),
            values,
            runtime);
        Compressor = new CompressorState(
            instanceId,
            dspPath.Append(StateNodeIds.Compressor),
            values,
            runtime);
        Equalizer = new EqualizerState(
            instanceId,
            dspPath.Append(StateNodeIds.Equalizer),
            values,
            runtime);
        EqualizerBanks = Enumerable.Range(0, DspConstants.BankCount)
            .Select(index => new EqualizerBankState(
                instanceId,
                dspPath
                    .Append(StateNodeIds.Equalizer)
                    .Append(StateNodeIds.EqualizerBank)
                    .Append(StateNodeIds.BankAt(index)),
                values,
                runtime,
                index,
                effectStatusSink))
            .ToArray();
        OutputGain = new GainState(
            instanceId,
            dspPath.Append(StateNodeIds.OutputGain),
            values,
            1.0F,
            value => runtime.OutputGain = value,
            value =>
            {
                runtime.OutputGainBypass = value;
                runtime.OutputGainActive = !value;
            });
    }

    public GainState InputGain { get; }
    public SaturatorState Saturator { get; }
    public CompressorState Compressor { get; }
    public EqualizerState Equalizer { get; }
    public EqualizerBankState[] EqualizerBanks { get; }
    public GainState OutputGain { get; }

}






