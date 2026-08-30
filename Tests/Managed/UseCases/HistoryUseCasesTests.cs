using System;
using System.Linq;

using Consolidator.Managed.Tests.Support;
using Consolidator.Managed.State;
using Consolidator.Managed.State.History;
using Consolidator.Managed.State.Tree;
using Xunit;

namespace Consolidator.Managed.Tests.UseCases;

using static ManagedApplicationFixture;

public sealed class HistoryUseCasesTests
{
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
    public void FramedEditAndHistoryJumpRestoreStateForEveryRegisteredClient()
    {
        using var application = new ManagedApplicationFixture();
        var editor = application.RegisterInstance();
        var observer = application.RegisterInstance();
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
            Symbol(editor.InstanceId.Value.ToString()),
            Symbol("41"),
            Integer(1),
            Symbol("entry"),
            Symbol("compressor"),
            Symbol("threshold"),
            Symbol("value"),
            Float(-18.0));
        application.Send(editor, "end_history", Symbol("41"));

        Assert.Equal(-18.0F, editor.Dsp.Latest.CompressorThresholdDb);
        Assert.Contains(
            observer.Output.Messages,
            message => message.Selector == "history_state" &&
                message.Atoms[3].Integer > 0 &&
                message.Atoms[4].Integer == 1);

        editor.Output.Clear();
        observer.Output.Clear();
        application.Send(editor, "jump_history", Integer(0));

        Assert.NotEqual(-18.0F, editor.Dsp.Latest.CompressorThresholdDb);
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
                "compressor.threshold");
        Assert.NotEqual(-18.0, snapshot.Atoms[8 + thresholdIndex * 6].Float);
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
}
