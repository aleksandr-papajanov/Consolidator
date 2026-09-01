using System;
using System.Linq;

using Consolidator.Managed.Tests.Support;
using Consolidator.Managed.State;
using Consolidator.Managed.State.History;
using Consolidator.Managed.State.Observers;
using Consolidator.Managed.State.Tree;
using Xunit;

namespace Consolidator.Managed.Tests.UseCases;

using static ManagedApplicationFixture;

public sealed class HistoryUseCasesTests
{
    [Fact]
    public void CurrentDspParameterIsRestoredByHistoryJump()
    {
        using var application = new ManagedApplicationFixture();
        var instance = application.RegisterInstance();

        application.Send(
            instance,
            "observe_target",
            Symbol(instance.InstanceId.Value.ToString()),
            Integer(0),
            Symbol("polish"));
        application.Send(instance, "begin_history", Symbol("7"));
        application.Send(
            instance,
            "write",
            Symbol("local"),
            Symbol("0"),
            Integer(1),
            Symbol("entry"),
            Symbol("polish"),
            Symbol("thick"),
            Symbol("value"),
            Float(0.75));
        application.Send(instance, "end_history", Symbol("7"));

        Assert.Equal(0.75F, instance.Dsp.Latest.PolishThick);

        application.Send(instance, "jump_history", Integer(0));

        Assert.Equal(0.0F, instance.Dsp.Latest.PolishThick);
    }

    [Fact]
    public void CallbacklessGestureParameterIsRestoredByHistoryJump()
    {
        using var application = new ManagedApplicationFixture();
        var instance = application.RegisterInstance();
        application.Send(
            instance,
            "observe_target",
            Symbol(instance.InstanceId.Value.ToString()),
            Integer(0),
            Symbol("polish"));

        application.Send(instance, "begin_history", Symbol("7"));
        application.Enqueue(
            instance,
            "write",
            Symbol("local"),
            Symbol("7"),
            Integer(1),
            Symbol("entry"),
            Symbol("polish"),
            Symbol("thick"),
            Symbol("value"),
            Float(0.75));
        Assert.True(System.Threading.SpinWait.SpinUntil(
            () => instance.Dsp.Latest.PolishThick == 0.75F,
            TimeSpan.FromSeconds(1)));
        application.Send(instance, "end_history", Symbol("7"));

        application.Send(instance, "jump_history", Integer(0));

        Assert.Equal(0.0F, instance.Dsp.Latest.PolishThick);
    }

    [Fact]
    public void EqualizerBankParameterIsRestoredByHistoryJump()
    {
        using var application = new ManagedApplicationFixture();
        var instance = application.RegisterInstance();
        application.Send(
            instance,
            "observe_target",
            Symbol(instance.InstanceId.Value.ToString()),
            Integer(0),
            Symbol("equalizer"));

        application.Send(instance, "begin_history", Symbol("7"));
        application.Send(
            instance,
            "write",
            Symbol("local"),
            Symbol("0"),
            Integer(1),
            Symbol("entry"),
            Symbol("equalizer"),
            Symbol("bank"),
            Integer(0),
            Symbol("filter"),
            Integer(3),
            Symbol("gain"),
            Symbol("value"),
            Float(9.0));
        application.Send(instance, "end_history", Symbol("7"));
        application.Send(instance, "jump_history", Integer(0));
        instance.Output.Clear();

        application.Send(
            instance,
            "read",
            Integer(1),
            Symbol("query"),
            Symbol("equalizer"),
            Symbol("bank"),
            Integer(0),
            Symbol("filter"),
            Integer(3),
            Symbol("gain"));

        Assert.Equal(0.0, instance.Output.Single("state_done").Atoms[^1].Float);
    }

    [Fact]
    public void RegistrationDoesNotCompareUnrelatedHistoryValues()
    {
        var history = new StateHistory();

        for (var index = 1; index <= 256; index++)
        {
            history.Register(new NonComparableHistoryValue(index));
        }
    }

    [Fact]
    public void ValueRegisteredAfterHistoryAdvanceUsesTheCurrentHistorySlot()
    {
        var history = new StateHistory();
        var registry = new StateRegistry<string>(history);
        registry.CreateRoot("instance");
        history.AdvanceHistoryPoint();

        var value = registry.CreateValue(
            "instance",
            new StatePath([new NodeId(1)]),
            string.Empty);
        value.Value = "Track";
        history.AdvanceHistoryPoint();

        Assert.True(history.JumpToHistory(1));
        Assert.Equal("Track", value.Value);
    }

    [Fact]
    public void BaselineReplacesEveryHistorySlotWhenCurrentValueAlreadyMatches()
    {
        var history = new StateHistory();
        var registry = new StateRegistry<string>(history);
        registry.CreateRoot("instance");
        var value = registry.CreateValue(
            "instance",
            new StatePath([new NodeId(1)]),
            0);

        history.AdvanceHistoryPoint();
        value.Value = 1;
        history.AdvanceHistoryPoint();
        value.Value = 2;
        Assert.True(history.JumpToHistory(1));
        Assert.Equal(1, value.Value);

        using (var transaction = history.BeginTransaction())
        {
            value.PrepareBaseline(1, transaction);
            transaction.Commit();
        }

        Assert.True(history.JumpToHistory(0));
        Assert.Equal(1, value.Value);
        Assert.True(history.JumpToHistory(2));
        Assert.Equal(1, value.Value);
    }

    [Fact]
    public void ObserverFailureDoesNotRollbackCommittedStorage()
    {
        var history = new StateHistory();
        var registry = new StateRegistry<string>(history);
        registry.CreateRoot("instance");
        var value = registry.CreateValue(
            "instance",
            new StatePath([new NodeId(1)]),
            0,
            new ThrowingObserver());

        using var transaction = history.BeginTransaction();
        value.Prepare(1, transaction);

        Assert.Throws<InvalidOperationException>(transaction.Commit);
        Assert.Equal(1, value.Value);

        using var nextTransaction = history.BeginTransaction();
        value.Prepare(2, nextTransaction);
        Assert.Throws<InvalidOperationException>(nextTransaction.Commit);
        Assert.Equal(2, value.Value);
    }

    [Fact]
    public void FramedEditAndHistoryJumpRestoreStateForEveryRegisteredClient()
    {
        using var application = new ManagedApplicationFixture();
        var editor = application.RegisterInstance();
        var observer = application.RegisterInstance();
        WriteGroup(application, editor, editor, 0, 1);
        WriteGroup(application, editor, observer, 0, 1);
        application.Send(
            observer,
            "observe_target",
            Symbol(editor.InstanceId.Value.ToString()),
            Integer(1),
            Symbol("compressor"));
        application.Send(
            editor,
            "observe_target",
            Symbol(editor.InstanceId.Value.ToString()),
            Integer(1),
            Symbol("compressor"));
        application.Send(editor, "set_instance_active", Integer(1));
        application.Send(observer, "set_instance_active", Integer(1));
        editor.Output.Clear();
        observer.Output.Clear();

        application.Send(editor, "begin_history", Symbol("41"));
        application.Enqueue(
            editor,
            "write",
            Symbol("group"),
            Symbol("41"),
            Integer(1),
            Symbol("entry"),
            Symbol("compressor"),
            Symbol("attack"),
            Symbol("value"),
            Float(0.5));
        application.Send(editor, "end_history", Symbol("41"));

        Assert.Equal(0.5F, editor.Dsp.Latest.CompressorAttack);
        Assert.Contains(
            observer.Output.Messages,
            message => message.Selector == "history_state" &&
                message.Atoms[3].Integer > 0 &&
                message.Atoms[4].Integer == 1);

        editor.Output.Clear();
        observer.Output.Clear();
        application.Send(editor, "jump_history", Integer(0));

        Assert.Equal(0.0F, editor.Dsp.Latest.CompressorAttack);
        Assert.Contains(
            observer.Output.Messages,
            message => message.Selector == "history_state" &&
                message.Atoms[2].Integer == 0 &&
                message.Atoms[4].Integer == 0 &&
            message.Atoms[5].Integer == 1);

        application.Send(editor, "set_instance_active", Integer(1));
        editor.Output.Clear();
        application.Send(
            editor,
            "observe_target",
            Symbol(editor.InstanceId.Value.ToString()),
            Integer(1),
            Symbol("compressor"));
        var snapshot = editor.Output.Single("target_state_snapshot");
        var thresholdIndex = Enumerable.Range(0, (int)snapshot.Atoms[6].Integer)
            .Single(index => snapshot.Atoms[7 + index * 6].Symbol ==
                "compressor.attack");
        Assert.Equal(0.0, snapshot.Atoms[8 + thresholdIndex * 6].Float);
    }

    private static void WriteGroup(
        ManagedApplicationFixture application,
        ManagedApplicationFixture.TestInstance source,
        ManagedApplicationFixture.TestInstance target,
        int bank,
        int group)
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
            Integer(bank),
            Symbol("group"),
            Symbol("value"),
            Integer(group));
    }

    private sealed class NonComparableHistoryValue : IHistoryValue
    {
        private readonly int _id;

        public NonComparableHistoryValue(int id)
        {
            _id = id;
        }

        public override bool Equals(object? obj)
        {
            throw new InvalidOperationException(
                "Unrelated history values must not be compared.");
        }

        public override int GetHashCode()
        {
            return _id;
        }

        public void SetCurrentSlot(int slot)
        {
        }

        public void CopySlot(int sourceSlot, int destinationSlot)
        {
        }

        public void ApplySlot(int slot)
        {
        }
    }

    private sealed class ThrowingObserver : IStateValueObserver<int>
    {
        public void Attach(StateValue<int> value)
        {
        }

        public void ValueChanged(
            StateValue<int> value,
            int previousValue,
            int currentValue)
        {
            throw new InvalidOperationException("Observer failed.");
        }

        public void Detach(StateValue<int> value)
        {
        }
    }
}
