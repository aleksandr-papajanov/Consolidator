using System;
using System.Threading;
using System.Threading.Tasks;
using Consolidator.Managed.Core;
using Consolidator.Managed.Core.Abstractions;
using Consolidator.Managed.Core.Instances;
using Consolidator.Managed.Native;
using Consolidator.Managed.Protocol;
using Microsoft.Extensions.DependencyInjection;
using Xunit;

namespace Consolidator.Managed.Tests;

public sealed class CoordinatorTests
{
    [Fact]
    public void ManagedServicesProvidesOneCoordinatorSingleton()
    {
        var first = ManagedServices.Provider.GetRequiredService<Coordinator>();
        var second = ManagedServices.Provider.GetRequiredService<Coordinator>();

        Assert.Same(first, second);
    }

    [Fact]
    public void CoordinatorIssuesDistinctIdsForMultipleInstances()
    {
        var coordinator = new Coordinator(new TestLogger());

        var first = coordinator.RegisterInstance(new TestOutput());
        var second = coordinator.RegisterInstance(new TestOutput());

        try
        {
            Assert.NotEqual(0UL, first);
            Assert.NotEqual(first, second);
        }
        finally
        {
            coordinator.UnregisterInstance(first);
            coordinator.UnregisterInstance(second);
        }
    }

    [Fact]
    public async Task UnregisterWaitsForActiveOutput()
    {
        using var outputEntered = new ManualResetEventSlim();
        using var releaseOutput = new ManualResetEventSlim();
        var output = new BlockingOutput(outputEntered, releaseOutput);
        var instance = new ConsolidatorInstance(1, output);

        var sendTask = Task.Run(() => instance.TrySend(
            "ready",
            Array.Empty<Atom>()));

        Assert.True(outputEntered.Wait(TimeSpan.FromSeconds(5)));

        var unregisterTask = Task.Run(instance.Stop);

        var completedTask = await Task.WhenAny(
            unregisterTask,
            Task.Delay(TimeSpan.FromMilliseconds(100)));

        Assert.NotSame(unregisterTask, completedTask);

        releaseOutput.Set();
        await sendTask;
        await unregisterTask;
    }

    private sealed class TestOutput : IInstanceOutput
    {
        public void Send(
            string selector,
            ReadOnlySpan<Atom> atoms)
        {
        }
    }

    private sealed class BlockingOutput : IInstanceOutput
    {
        private readonly ManualResetEventSlim _entered;
        private readonly ManualResetEventSlim _release;

        public BlockingOutput(
            ManualResetEventSlim entered,
            ManualResetEventSlim release)
        {
            _entered = entered;
            _release = release;
        }

        public void Send(
            string selector,
            ReadOnlySpan<Atom> atoms)
        {
            _entered.Set();
            _release.Wait();
        }
    }

    private sealed class TestLogger : IConsolidatorLogger
    {
        public void Info(string message)
        {
        }

        public void Warning(string message)
        {
        }

        public void Error(string message)
        {
        }
    }
}