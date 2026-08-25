using Consolidator.Integration.Tests.Support;
using Xunit;

namespace Consolidator.Integration.Tests.NativeAot;

using static InputAtom;

[Collection(NativeAotCollection.Name)]
public sealed class CommandRoundTripTests
{
    private readonly NativeLibraryFixture _library;

    public CommandRoundTripTests(NativeLibraryFixture library)
    {
        _library = library;
    }

    [Fact]
    public void NativeAtomsDriveManagedWriteReadAndNativeCallbackResponses()
    {
        using var instance = _library.Register();
        instance.ClearFrames();

        _library.Send(
            instance,
            "write",
            Integer(1),
            Symbol("ui"),
            Symbol("101"),
            Symbol(instance.InstanceId.ToString()),
            Symbol("0"),
            Integer(1),
            Symbol("entry"),
            Symbol("input_gain"),
            Symbol("gain"),
            Symbol("value"),
            Float(5.5));
        instance.WaitForResponse("101");

        Assert.Equal(5.5F, instance.PublishedGain);
        Assert.Equal(1, instance.Single("action_done").Atoms[^1].IntegerValue);
        var change = instance.Single("state_changed");
        Assert.Equal("input_gain.gain", change.Atoms[1].SymbolValue);
        Assert.Equal(5.5, change.Atoms[2].FloatValue);

        instance.ClearFrames();
        _library.Send(
            instance,
            "read",
            Integer(1),
            Symbol("ui"),
            Symbol("102"),
            Integer(1),
            Symbol("query"),
            Symbol("input_gain"),
            Symbol("gain"));
        instance.WaitForResponse("102");

        var response = instance.Single("state_done");
        Assert.Equal("102", response.Atoms[2].SymbolValue);
        Assert.Equal(5.5, response.Atoms[^1].FloatValue);
    }

    [Fact]
    public void UnregisterIsABarrierForLaterManagedOutput()
    {
        var instance = _library.Register();
        instance.ClearFrames();
        var instanceId = instance.InstanceId;
        instance.Dispose();

        _library.Send(
            instance,
            "initialize",
            Integer(1),
            Symbol("ui"),
            Symbol("201"));

        Assert.Equal(instanceId, instance.InstanceId);
        Assert.Empty(instance.Frames);
    }
}
