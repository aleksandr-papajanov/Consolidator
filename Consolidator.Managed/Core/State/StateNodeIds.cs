using Consolidator.Managed.State.Tree;

namespace Consolidator.Managed.Core.State;

public static class StateNodeIds
{
    public static readonly NodeId Instance = new(1);
    public static readonly NodeId Label = new(2);
    public static readonly NodeId Mute = new(4);
    public static readonly NodeId Solo = new(5);
    public static readonly NodeId Bank = new(6);
    public static readonly NodeId Group = new(8);
    public static readonly NodeId Dsp = new(9);
    public static readonly NodeId InputGain = new(10);
    public static readonly NodeId Saturator = new(11);
    public static readonly NodeId Compressor = new(12);
    public static readonly NodeId Equalizer = new(13);
    public static readonly NodeId OutputGain = new(14);
    public static readonly NodeId Gain = new(15);
    public static readonly NodeId Drive = new(16);
    public static readonly NodeId Output = new(17);
    public static readonly NodeId Mix = new(18);
    public static readonly NodeId DetectorAmount = new(19);
    public static readonly NodeId Threshold = new(20);
    public static readonly NodeId Ratio = new(21);
    public static readonly NodeId Attack = new(22);
    public static readonly NodeId Release = new(23);
    public static readonly NodeId Frequency = new(24);
    public static readonly NodeId Q = new(25);
    public static readonly NodeId Bypass = new(26);
    public static readonly NodeId Listen = new(27);
    public static readonly NodeId Detector = new(28);
    public static readonly NodeId EqualizerBank = new(29);
    public static readonly NodeId Filter = new(30);
    public static readonly NodeId FocusedBank = new(31);
    public static NodeId BankAt(int index) => new((uint)(100 + index));

    public static NodeId FilterAt(int index) => new((uint)(200 + index));
}




