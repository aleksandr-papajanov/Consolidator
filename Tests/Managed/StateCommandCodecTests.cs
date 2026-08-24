using Consolidator.Managed.Core.Commands.Definitions;
using Consolidator.Managed.Core.State;
using Consolidator.Managed.Protocol.Decoding;
using Consolidator.Managed.Protocol.Messages;
using Consolidator.Managed.State;
using Xunit;

namespace Consolidator.Managed.Tests;

public sealed class StateCommandCodecTests
{
    [Fact]
    public void DspPathStartsAtTheDspRoot()
    {
        var decoder = new StatePathDecoder();
        var path = decoder.Decode([
            Symbol("input_gain"),
            Symbol("gain")]);

        Assert.Equal(
            new StatePath([
                StateNodeIds.Dsp,
                StateNodeIds.InputGain,
                StateNodeIds.Gain]),
            path);
    }

    [Fact]
    public void NullGroupWritePreservesTheNullableGroupType()
    {
        var codec = new WriteInputCodec(new StatePathDecoder());
        var decoded = codec.Decode(
            [
                Symbol("0"),
                Symbol("0"),
                Integer(1),
                Symbol("entry"),
                Symbol("bank"),
                Integer(1),
                Symbol("group"),
                Symbol("value"),
                Symbol("none")
            ],
            new CommandFrameHeader(1, 2, "write", 0));

        var command = Assert.IsType<WriteStateCommand>(decoded.Command);
        Assert.Null(command.Value);
        Assert.Equal(typeof(GroupId?), command.ValueType);
    }

    private static Atom Integer(long value) =>
        new(AtomType.Integer, value, 0, null);

    private static Atom Symbol(string value) =>
        new(AtomType.Symbol, 0, 0, value);
}
