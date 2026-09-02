using Consolidator.Managed.Core.State;
using Consolidator.Managed.Core.State.Models;
using Consolidator.Managed.State;

namespace Consolidator.Managed.Core.Commands.Handlers;

internal static class ProcessorStateAccess
{
    public static StateValue<bool>? Bypass(ManagedState state, ProcessorId id) => id switch
    {
        ProcessorId.Input => state.Dsp.InputGain.Bypass,
        ProcessorId.Saturator => state.Dsp.Saturator.Bypass,
        ProcessorId.Compressor => state.Dsp.Compressor.Bypass,
        ProcessorId.Equalizer => state.Dsp.Equalizer.Bypass,
        ProcessorId.Polish => state.Dsp.Polish.Bypass,
        ProcessorId.Output => state.Dsp.OutputGain.Bypass,
        _ => null
    };
}
