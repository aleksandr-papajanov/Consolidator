using System;
using Consolidator.Managed.Core.State;
using Consolidator.Managed.Core.State.Bindings;
using Xunit;

namespace Consolidator.Managed.Tests;

public sealed class StateHistoryTests
{
    [Fact]
    public void AdvanceOpensPointAndRepeatedWritesCoalesce()
    {
        var history = new StateHistory();
        var value = history.CreateValue(new StateId(1), 0);

        history.AdvanceHistoryPoint();
        value.Value = 1;
        value.Value = 2;

        Assert.Equal(2, value.Value);
        Assert.True(history.CanUndo);

        Assert.True(history.Undo());
        Assert.Equal(0, value.Value);
    }

    [Fact]
    public void UndoAndRedoApplyTheSharedCursor()
    {
        var history = new StateHistory();
        var first = history.CreateValue(new StateId(1), 0);
        var second = history.CreateValue(new StateId(2), 10);

        history.AdvanceHistoryPoint();
        first.Value = 1;
        second.Value = 11;
        history.AdvanceHistoryPoint();
        first.Value = 2;
        second.Value = 12;

        Assert.True(history.Undo());
        Assert.Equal(1, first.Value);
        Assert.Equal(11, second.Value);

        Assert.True(history.Redo());
        Assert.Equal(2, first.Value);
        Assert.Equal(12, second.Value);
    }

    [Fact]
    public void NewAdvanceClearsRedoRange()
    {
        var history = new StateHistory();
        var value = history.CreateValue(new StateId(1), 0);

        history.AdvanceHistoryPoint();
        value.Value = 1;
        history.AdvanceHistoryPoint();
        value.Value = 2;
        Assert.True(history.Undo());

        history.AdvanceHistoryPoint();
        value.Value = 3;

        Assert.False(history.CanRedo);
        Assert.Equal(3, value.Value);
    }

    [Fact]
    public void FullRingRetainsOnlyTheNewestHistoryPoints()
    {
        var history = new StateHistory();
        var value = history.CreateValue(new StateId(1), 0);

        for (var point = 1; point <= StateHistory.Capacity; point++)
        {
            history.AdvanceHistoryPoint();
            value.Value = point;
        }

        var undoCount = 0;
        while (history.Undo())
        {
            undoCount++;
        }

        Assert.Equal(StateHistory.Capacity - 1, undoCount);
        Assert.Equal(1, value.Value);
    }

    [Fact]
    public void AdvancingAfterWritesDoesNotUndoThoseWrites()
    {
        var history = new StateHistory();
        var value = history.CreateValue(new StateId(1), 0);

        value.Value = 1;
        history.AdvanceHistoryPoint();

        Assert.True(history.Undo());
        Assert.Equal(1, value.Value);
    }

    [Fact]
    public void DisposedValueStopsReceivingHistoryProjectionCallbacks()
    {
        var history = new StateHistory();
        var firstApplyCount = 0;
        var secondApplyCount = 0;
        var first = history.CreateValue(
            new StateId(1),
            0,
            new StateBinding<int>(_ => firstApplyCount++));
        var second = history.CreateValue(
            new StateId(2),
            10,
            new StateBinding<int>(_ => secondApplyCount++));

        first.Dispose();

        Assert.Throws<ObjectDisposedException>(() => first.Value = 1);

        history.AdvanceHistoryPoint();
        second.Value = 20;
        history.AdvanceHistoryPoint();
        second.Value = 30;

        Assert.True(history.Undo());
        Assert.True(history.Redo());

        Assert.Equal(0, firstApplyCount);
        Assert.Equal(4, secondApplyCount);
        Assert.Equal(30, second.Value);
    }
}