using System;
using System.IO;
using System.Runtime.InteropServices;
using Xunit;

namespace Consolidator.Integration.Tests;

public sealed class ManagedNativeExportTests
{
    private static readonly string PublishedLibraryPath = Path.GetFullPath(
        Path.Combine(
            AppContext.BaseDirectory,
            "..",
            "..",
            "..",
            "..",
            "..",
            "Consolidator.Max",
            "externals",
            "Consolidator.Managed.dll"));

    [Fact]
    public void PublishedManagedLibrary_ExportsNativeEntryPoints()
    {
        Assert.True(
            File.Exists(PublishedLibraryPath),
            $"Publish Managed first: {PublishedLibraryPath}");

        nint library = NativeLibrary.Load(PublishedLibraryPath);
        try
        {
            var expectedExports = new[]
            {
                "ConsolidatorSetLogCallback",
                "ConsolidatorRegisterInstance",
                "ConsolidatorUnregisterInstance",
                "ConsolidatorSendMessage",
                "ConsolidatorPrepare",
                "ConsolidatorSendAudio"
            };

            foreach (var export in expectedExports)
            {
                Assert.True(
                    NativeLibrary.TryGetExport(library, export, out _),
                    $"Missing export: {export}");
            }
        }
        finally
        {
            NativeLibrary.Free(library);
        }
    }
}
