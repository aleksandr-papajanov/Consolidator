using Consolidator.Managed.State;

namespace Consolidator.Managed.Core.Commands.Accessors;

internal static class ProcessorStateAccess
{
    public static StateValue<bool>? Bypass(ManagedState state, ProcessorId id) => id switch
    {
        ProcessorId.GainStaging => state.Dsp.GainStaging.Bypass,
        ProcessorId.Saturator => state.Dsp.Saturator.Bypass,
        ProcessorId.Compressor => state.Dsp.Compressor.Bypass,
        ProcessorId.Equalizer => state.Dsp.Equalizer.Bypass,
        ProcessorId.Polish => state.Dsp.Polish.Bypass,
        _ => null
    };
}
