using Consolidator.Managed.Core.Dsp;
using Consolidator.Managed.Core.Services.Abstractions;
using Consolidator.Managed.Core.Settings;
using Consolidator.Managed.Core.State.Observers;
using Consolidator.Managed.State;

namespace Consolidator.Managed.Core.State.Models;

public sealed class DspState
{
    public DspState(
        InstanceId instanceId,
        StateValueFactory values,
        DspRuntimeState runtime,
        IActivityStatusSink activitySink)
    {
        ArgumentNullException.ThrowIfNull(values);
        ArgumentNullException.ThrowIfNull(runtime);

        var dspPath = new StatePath([StateNodeIds.Dsp]);
        InputGain = new InputState(instanceId, dspPath.Append(StateNodeIds.InputGain), values, runtime);
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
        Polish = new PolishState(
            instanceId,
            dspPath.Append(StateNodeIds.Polish),
            values,
            runtime);
        Equalizer = new EqualizerState(
            instanceId,
            dspPath.Append(StateNodeIds.Equalizer),
            values,
            runtime);
        Activity = new ActivityObserver(instanceId, activitySink);
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
                Activity))
            .ToArray();
        OutputGain = new OutputState(instanceId, dspPath.Append(StateNodeIds.OutputGain), values, runtime);
        Activity.Initialize(this);
    }

    public InputState InputGain { get; }
    public SaturatorState Saturator { get; }
    public CompressorState Compressor { get; }
    public PolishState Polish { get; }
    public EqualizerState Equalizer { get; }
    public EqualizerBankState[] EqualizerBanks { get; }
    internal ActivityObserver Activity { get; }

    public OutputState OutputGain { get; }
}






