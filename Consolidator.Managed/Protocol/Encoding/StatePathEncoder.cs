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
        if (node == StateNodeIds.InputGain) return "input_gain";
        if (node == StateNodeIds.OutputGain) return "output_gain";
        if (node == StateNodeIds.Saturator) return "saturator";
        if (node == StateNodeIds.Compressor) return "compressor";
        if (node == StateNodeIds.Equalizer) return "equalizer";
        if (node == StateNodeIds.Gain) return "gain";
        if (node == StateNodeIds.Drive) return "drive";
        if (node == StateNodeIds.Output) return "gain";
        if (node == StateNodeIds.Mix) return "mix";
        if (node == StateNodeIds.DetectorAmount) return "detector_amount";
        if (node == StateNodeIds.Threshold) return "threshold";
        if (node == StateNodeIds.Ratio) return "ratio";
        if (node == StateNodeIds.Attack) return "attack";
        if (node == StateNodeIds.Release) return "release";
        if (node == StateNodeIds.Frequency) return "frequency";
        if (node == StateNodeIds.Q) return "q";
        if (node == StateNodeIds.Bypass) return "bypass";
        if (node == StateNodeIds.Listen) return "listen";
        if (node == StateNodeIds.Detector) return "detector";
        throw new InvalidOperationException($"State node has no wire name: {node.Value}.");
    }
}
