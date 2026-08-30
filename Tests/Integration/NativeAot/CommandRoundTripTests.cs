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
            "observe_target",
            Integer(1),
            Symbol("ui"),
            Symbol("100"),
            Symbol(instance.InstanceId.ToString()),
            Integer(1),
            Symbol("equalizer"));
        instance.WaitForResponse("100");
        instance.ClearFrames();
        _library.Send(
            instance,
            "set_instance_active",
            Integer(1),
            Symbol("ui"),
            Symbol("99"),
            Integer(1));
        instance.WaitForResponse("99");
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
        var change = Assert.Single(
            instance.Frames,
            frame => frame.Selector == "state_changed" &&
                frame.Atoms[1].SymbolValue == "input_gain.gain");
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
    public void NativeAtomsDriveEqualizerBankActivityOffNotificationToNativeCallback()
    {
        using var instance = _library.Register();

        _library.Send(
            instance,
            "registry",
            Integer(1),
            Symbol("ui"),
            Symbol("1"));
        instance.WaitForResponse("1");
        instance.ClearFrames();

        _library.Send(
            instance,
            "write",
            Integer(1),
            Symbol("ui"),
            Symbol("2"),
            Symbol(instance.InstanceId.ToString()),
            Symbol("0"),
            Integer(1),
            Symbol("entry"),
            Symbol("equalizer"),
            Symbol("bank"),
            Integer(0),
            Symbol("filter"),
            Integer(1),
            Symbol("gain"),
            Symbol("value"),
            Float(4.5));
        instance.WaitForResponse("2");

        var activeNotification = Assert.Single(
            instance.Frames,
            frame => frame.Selector == "registry_bank_effect_changed");
        Assert.Equal(6, activeNotification.Atoms.Count);
        Assert.Equal(1, activeNotification.Atoms[0].IntegerValue);
        Assert.True(activeNotification.Atoms[1].IntegerValue <
            activeNotification.Atoms[2].IntegerValue);
        Assert.Equal(instance.InstanceId.ToString(), activeNotification.Atoms[3].SymbolValue);
        Assert.Equal(0, activeNotification.Atoms[4].IntegerValue);
        Assert.Equal(1, activeNotification.Atoms[5].IntegerValue);

        var activeProcessorNotification = Assert.Single(
            instance.Frames,
            frame => frame.Selector == "registry_processor_changed" &&
                frame.Atoms[4].SymbolValue == "equalizer");
        Assert.Equal(8, activeProcessorNotification.Atoms.Count);
        Assert.Equal(instance.InstanceId.ToString(), activeProcessorNotification.Atoms[3].SymbolValue);
        Assert.Equal(1, activeProcessorNotification.Atoms[5].IntegerValue);
        Assert.Equal(0, activeProcessorNotification.Atoms[6].IntegerValue);
        Assert.Equal(0, activeProcessorNotification.Atoms[7].IntegerValue);

        var activeRevision = activeNotification.Atoms[2].IntegerValue;

        instance.ClearFrames();
        _library.Send(
            instance,
            "write",
            Integer(1),
            Symbol("ui"),
            Symbol("3"),
            Symbol(instance.InstanceId.ToString()),
            Symbol("0"),
            Integer(1),
            Symbol("entry"),
            Symbol("equalizer"),
            Symbol("bank"),
            Integer(0),
            Symbol("filter"),
            Integer(1),
            Symbol("gain"),
            Symbol("value"),
            Float(0));
        instance.WaitForResponse("3");

        var notification = Assert.Single(
            instance.Frames,
            frame => frame.Selector == "registry_bank_effect_changed");
        Assert.Equal(6, notification.Atoms.Count);
        Assert.Equal(1, notification.Atoms[0].IntegerValue);
        Assert.True(activeRevision < notification.Atoms[1].IntegerValue);
        Assert.True(notification.Atoms[1].IntegerValue <
            notification.Atoms[2].IntegerValue);
        Assert.Equal(instance.InstanceId.ToString(), notification.Atoms[3].SymbolValue);
        Assert.Equal(0, notification.Atoms[4].IntegerValue);
        Assert.Equal(0, notification.Atoms[5].IntegerValue);

        var processorNotification = Assert.Single(
            instance.Frames,
            frame => frame.Selector == "registry_processor_changed" &&
                frame.Atoms[4].SymbolValue == "equalizer");
        Assert.Equal(0, processorNotification.Atoms[5].IntegerValue);
        Assert.Equal(0, processorNotification.Atoms[6].IntegerValue);
        Assert.Equal(0, processorNotification.Atoms[7].IntegerValue);
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
