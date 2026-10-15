using System.Runtime.InteropServices;

using Consolidator.Integration.Tests.Support;
using Xunit;

namespace Consolidator.Integration.Tests.NativeAot;

[Collection(NativeAotCollection.Name)]
public sealed class PublishedLibraryContractTests
{
    private readonly NativeLibraryFixture _library;

    public PublishedLibraryContractTests(NativeLibraryFixture library)
    {
        _library = library;
    }

    [Fact]
    public void PublishedLibraryExposesTheCompleteManagedNativeContract()
    {
        var exports = new[]
        {
            "ConsolidatorSetLogCallback",
            "ConsolidatorRegisterInstance",
            "ConsolidatorUnregisterInstance",
            "ConsolidatorSendMessage",
            "ConsolidatorPrepare",
            "ConsolidatorSendAudio",
            "ConsolidatorCapturePersistence",
            "ConsolidatorFreePersistence",
            "ConsolidatorRestorePersistence",
            "ConsolidatorShutdown"
        };

        foreach (var export in exports)
        {
            Assert.True(
                NativeLibrary.TryGetExport(_library.Library, export, out _),
                $"Missing export: {export}");
        }
    }

    [Fact]
    public void RegistrationPublishesInitialDspStateAndReturnsRealtimeHandle()
    {
        using var instance = _library.Register();

        Assert.NotEqual(0UL, instance.InstanceId);
        Assert.NotEqual((nuint)0, instance.AudioInputHandle);
        Assert.InRange(instance.PublishedSnapshotIndex, 0, 2);
        Assert.Equal(0.0F, instance.PublishedGain);
    }

    [Fact]
    public void RegistrationRestartsManagedServicesAfterShutdown()
    {
        using (var firstInstance = _library.Register())
        {
            Assert.NotEqual(0UL, firstInstance.InstanceId);
        }

        _library.ShutdownServices();

        using var restartedInstance = _library.Register();

        Assert.NotEqual(0UL, restartedInstance.InstanceId);
        Assert.NotEqual((nuint)0, restartedInstance.AudioInputHandle);
        Assert.Equal(0.0F, restartedInstance.PublishedGain);
    }

    [Fact]
    public void ActiveInstanceReceivesTelemetryFromItsSharedExchange()
    {
        using var instance = _library.Register();
        _library.Send(
            instance,
            "set_instance_active",
            InputAtom.Integer(1),
            InputAtom.Symbol("test"),
            InputAtom.Symbol("199"),
            InputAtom.Integer(1));
        instance.WaitForResponse("199");
        instance.ClearFrames();

        instance.PublishInputLevelTelemetry(2, -12.5F);

        var telemetry = instance.WaitForFrame("dsp_telemetry");
        Assert.Equal(8, telemetry.Atoms.Count);
        Assert.Equal(1, telemetry.Atoms[0].IntegerValue);
        Assert.Equal((long)instance.InstanceId, telemetry.Atoms[1].IntegerValue);
        Assert.Equal(2, telemetry.Atoms[2].IntegerValue);
        Assert.Equal(2, telemetry.Atoms[3].IntegerValue);
        Assert.Equal("gain_staging.input_level", telemetry.Atoms[4].SymbolValue);
        Assert.Equal(-12.5, telemetry.Atoms[5].FloatValue);
        Assert.Equal("gain_staging.input_level_fast", telemetry.Atoms[6].SymbolValue);
    }
}
