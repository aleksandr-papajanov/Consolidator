using Consolidator.Managed.Core.Settings;
using Consolidator.Managed.Core.State;
using Consolidator.Managed.Protocol.Messages;
using Consolidator.Managed.State;
using Consolidator.Managed.State.Tree;

namespace Consolidator.Managed.Protocol.Decoding;

internal sealed class StatePathDecoder : IStatePathDecoder
{
    public StatePath Decode(
        ReadOnlySpan<Atom> atoms,
        bool allowContainer = false)
    {
        if (atoms.IsEmpty)
        {
            throw new FormatException("State path is empty.");
        }

        var position = 0;
        var first = ReadSymbol(atoms, ref position);
        var path = new StatePath([StateNodeIds.Instance]);

        if (first == "dsp")
        {
            if (!allowContainer || position != atoms.Length)
            {
                throw new FormatException("Unexpected DSP state path segment.");
            }

            return new StatePath([StateNodeIds.Dsp]);
        }

        if (first is "label" or "mute" or "solo")
        {
            return path.Append(ToNode(first));
        }

        if (first == "bank")
        {
            var bank = ReadIndex(
                atoms,
                ref position,
                DspConstants.BankCount,
                wireBase: 0);
            path = path
                .Append(StateNodeIds.Bank)
                .Append(StateNodeIds.BankAt(bank));
            if (position < atoms.Length)
            {
                if (ReadSymbol(atoms, ref position) != "group")
                {
                    throw new FormatException("Unexpected bank path segment.");
                }

                return Finish(path.Append(StateNodeIds.Group), atoms, position);
            }

            if (allowContainer)
            {
                return path;
            }

            throw new FormatException("State path points to a bank container.");
        }

        path = new StatePath([
            StateNodeIds.Dsp,
            ToDeviceNode(first)]);
        while (position < atoms.Length)
        {
            var segment = ReadSymbol(atoms, ref position);
            if (segment == "detector")
            {
                path = path.Append(StateNodeIds.Detector);
                continue;
            }

            if (segment == "filter")
            {
                var filterCount = path.Nodes.Contains(StateNodeIds.Detector)
                    ? DspConstants.DetectorFilterCount
                    : DspConstants.EqualizerFilterCount;
                var filter = ReadIndex(
                    atoms,
                    ref position,
                    filterCount,
                    wireBase: 1);
                path = path
                    .Append(StateNodeIds.Filter)
                    .Append(StateNodeIds.FilterAt(filter));
                continue;
            }

            if (segment == "bank")
            {
                path = path
                    .Append(StateNodeIds.EqualizerBank);
                if (position < atoms.Length && atoms[position].Type == AtomType.Integer)
                {
                    var bank = ReadIndex(
                        atoms,
                        ref position,
                        DspConstants.BankCount,
                        wireBase: 0);
                    path = path.Append(StateNodeIds.BankAt(bank));
                }
                else
                {
                    path = path.Append(StateNodeIds.FocusedBank);
                }
                continue;
            }

            if (segment == "gain" && !path.Nodes.Contains(StateNodeIds.Filter))
            {
                throw new FormatException("Unknown state path segment: gain.");
            }

            path = path.Append(ToNode(segment));
            if (position != atoms.Length)
            {
                throw new FormatException("Unexpected state path segment.");
            }
        }

        if (!allowContainer && path.Depth <= 2)
        {
            throw new FormatException("State path points to a container.");
        }

        return path;
    }

    private static StatePath Finish(
        StatePath path,
        ReadOnlySpan<Atom> atoms,
        int position)
    {
        if (position != atoms.Length)
        {
            throw new FormatException("Unexpected state path segment.");
        }

        return path;
    }

    private static string ReadSymbol(ReadOnlySpan<Atom> atoms, ref int position)
    {
        if (position >= atoms.Length || atoms[position].Type != AtomType.Symbol)
        {
            throw new FormatException("Expected a symbol in state path.");
        }

        return atoms[position++].Symbol
            ?? throw new FormatException("State path symbol is null.");
    }

    private static int ReadIndex(
        ReadOnlySpan<Atom> atoms,
        ref int position,
        int count,
        int wireBase)
    {
        if (position >= atoms.Length || atoms[position].Type != AtomType.Integer)
        {
            throw new FormatException("Expected an index in state path.");
        }

        var value = atoms[position++].Integer;
        if (value < wireBase || value >= count + wireBase)
        {
            throw new FormatException("State path index is out of range.");
        }

        return (int)value - wireBase;
    }

    private static NodeId ToNode(string value) => value switch
    {
        "label" => StateNodeIds.Label,
        "mute" => StateNodeIds.Mute,
        "solo" => StateNodeIds.Solo,
        "level" => StateNodeIds.Level,
        "target" => StateNodeIds.Target,
        "width" => StateNodeIds.Width,
        "leveler" => StateNodeIds.Leveler,
        "drive" => StateNodeIds.Drive,
        "curve" => StateNodeIds.Curve,
        "split" => StateNodeIds.Split,
        "output" => StateNodeIds.Output,
        "gain" => StateNodeIds.Gain,
        "attack" => StateNodeIds.Attack,
        "sustain" => StateNodeIds.Sustain,
        "compression" => StateNodeIds.Compression,
        "character" => StateNodeIds.Character,
        "parallel" => StateNodeIds.Parallel,
        "thick" => StateNodeIds.Thick,
        "air" => StateNodeIds.Air,
        "limiter" => StateNodeIds.Limiter,
        "frequency" => StateNodeIds.Frequency,
        "q" => StateNodeIds.Q,
        "bypass" => StateNodeIds.Bypass,
        "listen" => StateNodeIds.Listen,
        _ => throw new FormatException($"Unknown state path segment: {value}.")
    };

    private static NodeId ToDeviceNode(string value) => value switch
    {
        "input_gain" => StateNodeIds.InputGain,
        "saturator" => StateNodeIds.Saturator,
        "compressor" => StateNodeIds.Compressor,
        "equalizer" => StateNodeIds.Equalizer,
        "polish" => StateNodeIds.Polish,
        "output_gain" => StateNodeIds.OutputGain,
        _ => throw new FormatException($"Unknown DSP path segment: {value}.")
    };
}
