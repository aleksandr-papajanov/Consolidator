using Consolidator.Managed.Core.Services.Instances;
using Consolidator.Managed.State.History;
using Consolidator.Managed.Tests.Support;

using Xunit;

using static Consolidator.Managed.Tests.Support.ManagedApplicationFixture;

namespace Consolidator.Managed.Tests.UseCases;

public sealed class FrontendIntentUseCasesTests
{
    [Fact]
    public void DiscreteStateWriteOwnsOneHistoryPointWithoutFrontendHistoryCommands()
    {
        using var application = new ManagedApplicationFixture();
        var instance = application.RegisterInstance();
        var history = application.GetRequiredService<StateHistory>();

        WriteOutputLeveler(application, instance, true);

        Assert.Equal(1, history.Snapshot.Cursor);
        Assert.Equal(1U, instance.Dsp.Latest.OutputLeveler);

        WriteOutputLeveler(application, instance, true);

        Assert.Equal(1, history.Snapshot.Cursor);
        application.Send(instance, "jump_history", Integer(0));
        Assert.Equal(0U, instance.Dsp.Latest.OutputLeveler);
        application.Send(instance, "jump_history", Integer(1));
        Assert.Equal(1U, instance.Dsp.Latest.OutputLeveler);
    }

    [Fact]
    public void GroupIntentAllocatesAndRestoresAllMembershipsAsOneHistoryStep()
    {
        using var application = new ManagedApplicationFixture();
        var first = application.RegisterInstance();
        var second = application.RegisterInstance();
        var registry = application.GetRequiredService<InstanceRegistry>();
        var history = application.GetRequiredService<StateHistory>();
        var firstGroup = registry.FindInstance(first.InstanceId)!.State.Instance.Banks[1].Group;
        var secondGroup = registry.FindInstance(second.InstanceId)!.State.Instance.Banks[2].Group;

        application.Send(first, "group_banks", Integer(2),
            Symbol(first.InstanceId.Value.ToString()), Integer(1),
            Symbol(second.InstanceId.Value.ToString()), Integer(2));

        Assert.Equal(1U, firstGroup.Value!.Value.Value);
        Assert.Equal(firstGroup.Value, secondGroup.Value);
        Assert.Equal(1, history.Snapshot.Cursor);

        application.Send(first, "jump_history", Integer(0));
        Assert.Null(firstGroup.Value);
        Assert.Null(secondGroup.Value);
        application.Send(first, "jump_history", Integer(1));
        Assert.Equal(1U, firstGroup.Value!.Value.Value);
        Assert.Equal(firstGroup.Value, secondGroup.Value);

        application.Send(first, "ungroup_banks", Symbol(first.InstanceId.Value.ToString()), Integer(1));
        Assert.Null(firstGroup.Value);
        Assert.Null(secondGroup.Value);
        Assert.Equal(2, history.Snapshot.Cursor);
        application.Send(first, "jump_history", Integer(1));
        Assert.Equal(firstGroup.Value, secondGroup.Value);
        Assert.Equal(1U, firstGroup.Value!.Value.Value);
    }

    [Fact]
    public void InvalidGroupSelectionChangesNeitherMembershipNorHistory()
    {
        using var application = new ManagedApplicationFixture();
        var first = application.RegisterInstance();
        var second = application.RegisterInstance();
        var registry = application.GetRequiredService<InstanceRegistry>();
        var history = application.GetRequiredService<StateHistory>();

        // The last bank belongs to the automatic group and cannot be regrouped.
        application.Send(first, "group_banks", Integer(2),
            Symbol(first.InstanceId.Value.ToString()), Integer(1),
            Symbol(second.InstanceId.Value.ToString()), Integer(6));

        Assert.Null(registry.FindInstance(first.InstanceId)!.State.Instance.Banks[1].Group.Value);
        Assert.Equal(0, history.Snapshot.Cursor);
        Assert.Equal(0, first.Output.Single("action_done").Atoms[^1].Integer);
    }

    private static void WriteOutputLeveler(
        ManagedApplicationFixture application,
        TestInstance instance,
        bool enabled)
    {
        application.Send(instance, "write", Symbol("local"), Symbol("0"), Integer(1),
            Symbol("entry"), Symbol("gain_staging"), Symbol("output_leveler"),
            Symbol("value"), Integer(enabled ? 1 : 0), Symbol("copy"));
    }
}
