using System;
using System.Diagnostics;
using System.Linq;
using System.Threading;

using Consolidator.Managed.Tests.Support;
using Xunit;
using Xunit.Abstractions;

namespace Consolidator.Managed.Tests.UseCases;

using static ManagedApplicationFixture;

public sealed class BankApproximationUseCasesTests
{
    private readonly ITestOutputHelper _output;

    public BankApproximationUseCasesTests(ITestOutputHelper output)
    {
        _output = output;
    }

    [Fact]
    public void CommitRejectsAnEmptySourceWithoutChangingBankZero()
    {
        using var application = new ManagedApplicationFixture();
        var instance = application.RegisterInstance();
        FocusBank(application, instance, 2);
        instance.Output.Clear();

        application.Send(
            instance,
            "commit_bank",
            Symbol(instance.InstanceId.Value.ToString()),
            Integer(2),
            Symbol("local"));

        Assert.Equal(0, instance.Output.Single("action_done").Atoms[^1].Integer);
        Assert.All(
            Enumerable.Range(1, 7),
            filter => Assert.Equal(0.0, ReadGain(application, instance, 0, filter)));
    }

    [Fact]
    public void CommitApproximatesTheFocusedBankIntoBankZeroAndResetsTheSource()
    {
        using var application = new ManagedApplicationFixture();
        var instance = application.RegisterInstance();
        FocusBank(application, instance, 2);
        WriteGain(application, instance, 2, 1, 6.0);
        instance.Output.Clear();

        application.Send(
            instance,
            "commit_bank",
            Symbol(instance.InstanceId.Value.ToString()),
            Integer(2),
            Symbol("local"));

        Assert.Equal(1, instance.Output.Single("action_done").Atoms[^1].Integer);
        Assert.Equal(0.0, ReadGain(application, instance, 2, 1));
        Assert.Contains(
            Enumerable.Range(1, 7),
            filter => Math.Abs(ReadGain(application, instance, 0, filter)) > 0.1);
    }

    [Fact]
    public void GroupCommitCalculatesAnIndependentBankZeroForEveryMember()
    {
        using var application = new ManagedApplicationFixture();
        var first = application.RegisterInstance();
        var second = application.RegisterInstance();
        WriteGroup(application, first, first, 2, 4);
        WriteGroup(application, first, second, 3, 4);
        FocusBank(application, first, 2);
        WriteGain(application, first, 2, 1, 6.0);
        FocusBank(application, second, 3);
        WriteGain(application, second, 3, 1, -6.0);
        FocusBank(application, first, 2);
        first.Output.Clear();

        application.Send(
            first,
            "commit_bank",
            Symbol(first.InstanceId.Value.ToString()),
            Integer(2),
            Symbol("group"));

        Assert.Equal(0.0, ReadGain(application, first, 2, 1));
        Assert.Equal(0.0, ReadGain(application, second, 3, 1));
        Assert.Contains(
            Enumerable.Range(1, 7),
            filter => ReadGain(application, first, 0, filter) > 0.1);
        Assert.Contains(
            Enumerable.Range(1, 7),
            filter => ReadGain(application, second, 0, filter) < -0.1);
    }

    [Fact]
    public void RefineCompletesWithinTheManagedOperationBudget()
    {
        using var application = new ManagedApplicationFixture();
        var instance = application.RegisterInstance();
        FocusBank(application, instance, 3);
        WriteGain(application, instance, 3, 1, 6.0);
        instance.Output.Clear();

        var stopwatch = Stopwatch.StartNew();
        var requestId = application.Enqueue(
            instance,
            "refine_bank",
            Symbol(instance.InstanceId.Value.ToString()),
            Integer(3),
            Symbol("local"));
        Assert.True(
            SpinWait.SpinUntil(
                () => instance.Output.Messages.Any(message =>
                    message.Selector == "action_done" &&
                    message.Atoms.Count > 2 &&
                    message.Atoms[2].Symbol == requestId.ToString()),
                TimeSpan.FromSeconds(3)),
            $"No terminal response was received for request {requestId}.");
        stopwatch.Stop();

        var action = instance.Output.Single("action_done");
        Assert.Equal(1, action.Atoms[^1].Integer);
        Assert.InRange(stopwatch.Elapsed, TimeSpan.Zero, TimeSpan.FromSeconds(3));
        _output.WriteLine($"Refine duration: {stopwatch.Elapsed.TotalMilliseconds:F0} ms");
    }

    [Fact]
    public void RefineMeasuresFullBankDuration()
    {
        using var application = new ManagedApplicationFixture();
        var instance = application.RegisterInstance();
        FocusBank(application, instance, 3);
        var gains = new[] { 2.0, 3.0, -2.0, 2.5, -3.0, 1.5, -1.5 };
        for (var filter = 0; filter < gains.Length; filter++)
        {
            WriteGain(application, instance, 3, filter + 1, gains[filter]);
        }
        instance.Output.Clear();

        var stopwatch = Stopwatch.StartNew();
        var requestId = application.Enqueue(
            instance,
            "refine_bank",
            Symbol(instance.InstanceId.Value.ToString()),
            Integer(3),
            Symbol("local"));
        Assert.True(
            SpinWait.SpinUntil(
                () => instance.Output.Messages.Any(message =>
                    message.Selector == "action_done" &&
                    message.Atoms.Count > 2 &&
                    message.Atoms[2].Symbol == requestId.ToString()),
                TimeSpan.FromMinutes(2)),
            $"No terminal response was received for request {requestId}.");
        stopwatch.Stop();

        var action = instance.Output.Single("action_done");
        Assert.Equal(1, action.Atoms[^1].Integer);
        _output.WriteLine($"Full-bank Refine duration: {stopwatch.Elapsed.TotalMilliseconds:F0} ms");
    }

    private static void FocusBank(
        ManagedApplicationFixture application,
        ManagedApplicationFixture.TestInstance instance,
        int bankIndex)
    {
        application.Send(
            instance,
            "observe_target",
            Symbol(instance.InstanceId.Value.ToString()),
            Integer(bankIndex),
            Symbol("equalizer"));
    }

    private static void WriteGain(
        ManagedApplicationFixture application,
        ManagedApplicationFixture.TestInstance instance,
        int bankIndex,
        int filterId,
        double gainDb)
    {
        application.Send(
            instance,
            "write",
            Symbol("local"),
            Symbol("0"),
            Integer(1),
            Symbol("entry"),
            Symbol("equalizer"),
            Symbol("bank"),
            Integer(bankIndex),
            Symbol("filter"),
            Integer(filterId),
            Symbol("gain"),
            Symbol("value"),
            Float(gainDb));
    }

    private static double ReadGain(
        ManagedApplicationFixture application,
        ManagedApplicationFixture.TestInstance instance,
        int bankIndex,
        int filterId)
    {
        instance.Output.Clear();
        application.Send(
            instance,
            "read",
            Integer(1),
            Symbol("query"),
            Symbol("equalizer"),
            Symbol("bank"),
            Integer(bankIndex),
            Symbol("filter"),
            Integer(filterId),
            Symbol("gain"));
        return instance.Output.Single("state_done").Atoms[^1].Float;
    }

    private static void WriteGroup(
        ManagedApplicationFixture application,
        ManagedApplicationFixture.TestInstance source,
        ManagedApplicationFixture.TestInstance target,
        int bankIndex,
        int groupId)
    {
        application.Send(
            source,
            "write",
            Symbol("topology"),
            Symbol(target.InstanceId.Value.ToString()),
            Symbol("0"),
            Integer(1),
            Symbol("entry"),
            Symbol("bank"),
            Integer(bankIndex),
            Symbol("group"),
            Symbol("value"),
            Integer(groupId));
    }
}
