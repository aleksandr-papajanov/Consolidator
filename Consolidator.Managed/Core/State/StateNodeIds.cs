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
    public static readonly NodeId Level = new(15);
    public static readonly NodeId Drive = new(16);
    public static readonly NodeId Output = new(17);
    public static readonly NodeId Attack = new(22);
    public static readonly NodeId Frequency = new(24);
    public static readonly NodeId Q = new(25);
    public static readonly NodeId Bypass = new(26);
    public static readonly NodeId Detector = new(28);
    public static readonly NodeId EqualizerBank = new(29);
    public static readonly NodeId Filter = new(30);
    public static readonly NodeId FocusedBank = new(31);
    public static readonly NodeId Target = new(32);
    public static readonly NodeId Width = new(33);
    public static readonly NodeId Leveler = new(34);
    public static readonly NodeId Curve = new(35);
    public static readonly NodeId Split = new(36);
    public static readonly NodeId Sustain = new(37);
    public static readonly NodeId Compression = new(38);
    public static readonly NodeId Character = new(39);
    public static readonly NodeId Parallel = new(40);
    public static readonly NodeId Thick = new(41);
    public static readonly NodeId Air = new(42);
    public static readonly NodeId Limiter = new(43);
    public static readonly NodeId Polish = new(44);
    public static readonly NodeId Gain = new(45);
    public static NodeId BankAt(int index) => new((uint)(100 + index));

    public static NodeId FilterAt(int index) => new((uint)(200 + index));
}




