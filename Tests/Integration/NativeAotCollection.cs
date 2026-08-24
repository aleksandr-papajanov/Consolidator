using Consolidator.Integration.Tests.Support;
using Xunit;

namespace Consolidator.Integration.Tests;

[CollectionDefinition(Name, DisableParallelization = true)]
public sealed class NativeAotCollection : ICollectionFixture<NativeLibraryFixture>
{
    public const string Name = "NativeAOT boundary";
}
