using Consolidator.Managed.Core.State;
using Consolidator.Managed.State;
using Consolidator.Managed.State.Tree;

namespace Consolidator.Managed.Protocol.Encoding;

internal static class StatePathEncoder
{
    public static string Encode(StatePath path, int? observedBankId = null)
    {
        var parts = new List<string>();
        for (var index = 0; index < path.Nodes.Count; index++)
        {
            var node = path.Nodes[index];
            if (node == StateNodeIds.Instance || node == StateNodeIds.Dsp) continue;
            if (node == StateNodeIds.Bank || node == StateNodeIds.EqualizerBank)
            {
                var bank = (int)path.Nodes[++index].Value - 100;
                parts.Add("bank");
                if (node == StateNodeIds.Bank || observedBankId != bank)
                {
                    parts.Add(bank.ToString());
                }
                continue;
            }
            if (node == StateNodeIds.Filter)
            {
                parts.Add("filter");
                parts.Add(((int)path.Nodes[++index].Value - 199).ToString());
                continue;
            }
            parts.Add(ToSymbol(node));
        }
        return string.Join(".", parts).Replace(
            ".bank.filter.",
            ".filter.",
            StringComparison.Ordinal);
    }

    private static string ToSymbol(NodeId node)
    {
        if (node == StateNodeIds.Label) return "label";
        if (node == StateNodeIds.Mute) return "mute";
        if (node == StateNodeIds.Solo) return "solo";
        if (node == StateNodeIds.Group) return "group";
        if (node == StateNodeIds.GainStaging) return "gain_staging";
        if (node == StateNodeIds.Saturator) return "saturator";
        if (node == StateNodeIds.Shaper) return "shaper";
        if (node == StateNodeIds.Compression) return "compression";
        if (node == StateNodeIds.Equalizer) return "equalizer";
        if (node == StateNodeIds.Polish) return "polish";
        if (node == StateNodeIds.Gain) return "gain";
        if (node == StateNodeIds.Drive) return "drive";
        if (node == StateNodeIds.Output) return "output";
        if (node == StateNodeIds.Width) return "width";
        if (node == StateNodeIds.InputLevel) return "input_level";
        if (node == StateNodeIds.InputTarget) return "input_target";
        if (node == StateNodeIds.InputLeveler) return "input_leveler";
        if (node == StateNodeIds.OutputLevel) return "output_level";
        if (node == StateNodeIds.OutputTarget) return "output_target";
        if (node == StateNodeIds.OutputLeveler) return "output_leveler";
        if (node == StateNodeIds.Limit) return "limit";
        if (node == StateNodeIds.Curve) return "curve";
        if (node == StateNodeIds.Split) return "split";
        if (node == StateNodeIds.Attack) return "attack";
        if (node == StateNodeIds.Sustain) return "sustain";
        if (node == StateNodeIds.Reduction) return "reduction";
        if (node == StateNodeIds.Type) return "type";
        if (node == StateNodeIds.Parallel) return "parallel";
        if (node == StateNodeIds.Release) return "release";
        if (node == StateNodeIds.Ratio) return "ratio";
        if (node == StateNodeIds.Profile) return "profile";
        if (node == StateNodeIds.ProfileSsl) return "ssl";
        if (node == StateNodeIds.Profile76) return "76";
        if (node == StateNodeIds.Profile3A) return "3a";
        if (node == StateNodeIds.Profile2A) return "2a";
        if (node == StateNodeIds.Thick) return "thick";
        if (node == StateNodeIds.Air) return "air";
        if (node == StateNodeIds.Frequency) return "frequency";
        if (node == StateNodeIds.Q) return "q";
        if (node == StateNodeIds.Bypass) return "bypass";
        if (node == StateNodeIds.InputDetector) return "input_detector";
        if (node == StateNodeIds.OutputDetector) return "output_detector";
        if (node == StateNodeIds.Detector) return "detector";
        throw new InvalidOperationException($"State node has no wire name: {node.Value}.");
    }
}
