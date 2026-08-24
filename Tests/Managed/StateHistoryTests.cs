using Consolidator.Managed.State;
using Consolidator.Managed.State.History;
using Consolidator.Managed.State.Observers;
using Consolidator.Managed.State.Tree;
using Xunit;

namespace Consolidator.Managed.Tests;

public sealed class StateHistoryTests
{
    [Fact]
    public void RegistrySupportsArbitraryRootIdentifiers()
    {
        var history = new StateHistory();
        var registry = new StateRegistry<string>(history);
        var root = registry.CreateRoot("root-a");
        var path = new StatePath([new NodeId(7)]);
        var value = registry.CreateValue("root-a", path, 10);
        var node = Assert.IsAssignableFrom<IStateNode<int>>(root.Find(path));

        Assert.Equal(10, node.Value);
        Assert.Equal(StateWriteStatus.Applied, node.Write(20));
        Assert.Equal(20, value.Value);

        registry.RemoveRoot("root-a");

        Assert.Throws<ObjectDisposedException>(() => value.Value = 30);
    }

    [Fact]
    public void AdvanceOpensPointAndRepeatedWritesCoalesce()
    {
        var history = new StateHistory();
        using var value = CreateValue(history, 0);

        history.AdvanceHistoryPoint();
        value.Value = 1;
        value.Value = 2;

        Assert.Equal(2, value.Value);
        Assert.True(history.CanUndo);
        Assert.True(history.JumpToHistory(0));
        Assert.Equal(0, value.Value);
    }

    [Fact]
    public void UndoAndRedoApplyTheSharedCursor()
    {
        var history = new StateHistory();
        using var first = CreateValue(history, 0);
        using var second = CreateValue(history, 10);

        history.AdvanceHistoryPoint();
        first.Value = 1;
        second.Value = 11;
        history.AdvanceHistoryPoint();
        first.Value = 2;
        second.Value = 12;

        Assert.True(history.JumpToHistory(1));
        Assert.Equal(1, first.Value);
        Assert.Equal(11, second.Value);
        Assert.True(history.JumpToHistory(2));
        Assert.Equal(2, first.Value);
        Assert.Equal(12, second.Value);
    }

    [Fact]
    public void NewAdvanceClearsRedoRange()
    {
        var history = new StateHistory();
        using var value = CreateValue(history, 0);

        history.AdvanceHistoryPoint();
        value.Value = 1;
        history.AdvanceHistoryPoint();
        value.Value = 2;
        Assert.True(history.JumpToHistory(1));

        history.AdvanceHistoryPoint();
        value.Value = 3;

        Assert.False(history.CanRedo);
        Assert.Equal(3, value.Value);
    }

    [Fact]
    public void FullRingRetainsOnlyTheNewestHistoryPoints()
    {
        var history = new StateHistory();
        using var value = CreateValue(history, 0);

        for (var point = 1; point <= StateHistory.Capacity; point++)
        {
            history.AdvanceHistoryPoint();
            value.Value = point;
        }

        Assert.True(history.JumpToHistory(0));
        Assert.Equal(1, value.Value);
    }

    [Fact]
    public void AdvancingAfterWritesDoesNotUndoThoseWrites()
    {
        var history = new StateHistory();
        using var value = CreateValue(history, 0);

        value.Value = 1;
        history.AdvanceHistoryPoint();

        Assert.True(history.Undo());
        Assert.Equal(1, value.Value);
    }

    [Fact]
    public void DisposedValueStopsReceivingObserverCallbacks()
    {
        var history = new StateHistory();
        var firstApplyCount = 0;
        var secondApplyCount = 0;
        var first = CreateValue(
            history,
            0,
            new StateProjectionObserver<int>(_ => firstApplyCount++));
        using var second = CreateValue(
            history,
            10,
            new StateProjectionObserver<int>(_ => secondApplyCount++));

        first.Dispose();
        Assert.Throws<ObjectDisposedException>(() => first.Value = 1);

        history.AdvanceHistoryPoint();
        second.Value = 20;
        history.AdvanceHistoryPoint();
        second.Value = 30;

        Assert.True(history.JumpToHistory(1));
        Assert.True(history.JumpToHistory(2));
        Assert.Equal(1, firstApplyCount);
        Assert.Equal(5, secondApplyCount);
        Assert.Equal(30, second.Value);
    }

    private static StateValue<TValue> CreateValue<TValue>(
        StateHistory history,
        TValue initialValue,
        params IStateValueObserver<TValue>[] observers)
    {
        var value = new StateValue<TValue>(
            initialValue,
            observers,
            history.Unregister);
        history.Register(value);
        return value;
    }
}
