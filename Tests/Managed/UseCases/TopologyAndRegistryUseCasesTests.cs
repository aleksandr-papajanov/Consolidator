using System.Linq;

using Consolidator.Managed.Tests.Support;
using Xunit;

namespace Consolidator.Managed.Tests.UseCases;

using static ManagedApplicationFixture;

public sealed class TopologyAndRegistryUseCasesTests
{
    [Fact]
    public void GroupedBanksPropagateEditsAndRegistryReportsTheConnectedTopology()
    {
        using var application = new ManagedApplicationFixture();
        var first = application.RegisterInstance();
        var second = application.RegisterInstance();
        first.Output.Clear();
        second.Output.Clear();

        WriteGroup(application, first, first, 1, 7);
        WriteGroup(application, first, second, 2, 7);
        application.Send(
            first,
            "observe_target",
            Symbol(first.InstanceId.Value.ToString()),
            Integer(1));
        application.Send(
            second,
            "observe_target",
            Symbol(second.InstanceId.Value.ToString()),
            Integer(2));
        first.Output.Clear();
        second.Output.Clear();

        WriteFilterGain(application, first, first, 1, 4.5);

        Assert.Contains(
            first.Output.Messages,
            message => IsFilterGainChange(message, 4.5));
        Assert.Contains(
            second.Output.Messages,
            message => IsFilterGainChange(message, 4.5));

        first.Output.Clear();
        application.Send(first, "registry");
        var messages = first.Output.Messages;
        Assert.Equal(2, messages.Count(message => message.Selector == "registry_instance"));
        Assert.Contains(
            messages,
            message => message.Selector == "registry_member" &&
                message.Atoms[3].Integer == 7 &&
                message.Atoms[4].Symbol == first.InstanceId.Value.ToString() &&
                message.Atoms[5].Integer == 1);
        Assert.Contains(
            messages,
            message => message.Selector == "registry_member" &&
                message.Atoms[3].Integer == 7 &&
                message.Atoms[4].Symbol == second.InstanceId.Value.ToString() &&
                message.Atoms[5].Integer == 2);
    }

    [Fact]
    public void ObserveTargetReturnsACompleteBankRelativeSnapshot()
    {
        using var application = new ManagedApplicationFixture();
        var source = application.RegisterInstance();
        var target = application.RegisterInstance();
        source.Output.Clear();

        application.Send(
            source,
            "observe_target",
            Symbol(target.InstanceId.Value.ToString()),
            Integer(3));

        var begin = source.Output.Single("target_state_begin");
        var done = source.Output.Single("target_state_done");
        var entries = source.Output.Messages
            .Where(message => message.Selector == "target_state_entry")
            .ToArray();
        Assert.Equal(target.InstanceId.Value.ToString(), begin.Atoms[3].Symbol);
        Assert.Equal(3, begin.Atoms[4].Integer);
        Assert.NotEmpty(entries);
        Assert.Equal(begin.Atoms[5].Integer, entries.Length);
        Assert.Equal(begin.Atoms[5].Integer, done.Atoms[5].Integer);
        Assert.Contains(
            entries,
            entry => entry.Atoms[4].Symbol == "equalizer.filter.1.gain");
    }

    private static void WriteGroup(
        ManagedApplicationFixture application,
        TestInstance source,
        TestInstance target,
        int bank,
        int group)
    {
        application.Send(
            source,
            "write",
            Symbol(target.InstanceId.Value.ToString()),
            Symbol("0"),
            Integer(1),
            Symbol("entry"),
            Symbol("bank"),
            Integer(bank),
            Symbol("group"),
            Symbol("value"),
            Integer(group));
    }

    private static void WriteFilterGain(
        ManagedApplicationFixture application,
        TestInstance source,
        TestInstance target,
        int bank,
        double gain)
    {
        application.Send(
            source,
            "write",
            Symbol(target.InstanceId.Value.ToString()),
            Symbol("0"),
            Integer(1),
            Symbol("entry"),
            Symbol("equalizer"),
            Symbol("bank"),
            Integer(bank),
            Symbol("filter"),
            Integer(1),
            Symbol("gain"),
            Symbol("value"),
            Float(gain));
    }

    private static bool IsFilterGainChange(
        Consolidator.Managed.Protocol.Messages.ProtocolOutput message,
        double expected) =>
        message.Selector == "state_changed" &&
        message.Atoms[1].Symbol == "equalizer.filter.1.gain" &&
        message.Atoms[2].Float == expected;
}
