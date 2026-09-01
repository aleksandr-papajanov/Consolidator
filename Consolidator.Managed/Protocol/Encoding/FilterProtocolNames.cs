using Consolidator.Managed.Core.Settings;
using Consolidator.Managed.Core.State;
using Consolidator.Managed.State.Tree;

namespace Consolidator.Managed.Protocol.Encoding;

internal static class FilterProtocolNames
{
    public const string Gain = "gain";
    public const string Tilt = "tilt";
    public const string LowShelf = "low_shelf";
    public const string HighShelf = "high_shelf";
    public const string Bell = "bell";

    public static string GetParameter(NodeId node) => node == StateNodeIds.Frequency
        ? "frequency"
        : node == StateNodeIds.Q
            ? "q"
            : "gain";

    public static string Get(FilterDefinition definition) => definition switch
    {
        GainFilterDefinition => Gain,
        TiltFilterDefinition => Tilt,
        LowShelfFilterDefinition => LowShelf,
        HighShelfFilterDefinition => HighShelf,
        BellFilterDefinition => Bell,
        _ => throw new InvalidOperationException("Unknown filter definition.")
    };
}