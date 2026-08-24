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
        Assert.Equal(1.0F, instance.PublishedGain);
    }
}
