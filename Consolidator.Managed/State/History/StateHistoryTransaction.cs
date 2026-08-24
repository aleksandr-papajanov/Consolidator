namespace Consolidator.Managed.State.History;

public sealed class StateHistoryTransaction : IDisposable
{
    private readonly List<IStateTransactionEntry> _entries = new();
    private readonly List<Action> _committedChanges = new();
    private bool _completed;

    internal void Add(IStateTransactionEntry entry)
    {
        ObjectDisposedException.ThrowIf(_completed, this);

        if (!_entries.Contains(entry))
        {
            _entries.Add(entry);
        }
    }

    internal void AddCommittedChange(Action change)
    {
        ArgumentNullException.ThrowIfNull(change);
        ObjectDisposedException.ThrowIf(_completed, this);
        _committedChanges.Add(change);
    }

    public void Commit()
    {
        ObjectDisposedException.ThrowIf(_completed, this);

        var committedCount = 0;
        try
        {
            foreach (var entry in _entries)
            {
                committedCount++;
                entry.Commit();
            }

            foreach (var entry in _entries)
            {
                entry.Complete();
            }

            _completed = true;
        }
        catch
        {
            try
            {
                RollbackEntries(committedCount);
            }
            finally
            {
                _completed = true;
            }

            throw;
        }

        foreach (var change in _committedChanges)
        {
            change();
        }
    }

    public void Rollback()
    {
        if (_completed)
        {
            return;
        }

        try
        {
            RollbackEntries(_entries.Count);
        }
        finally
        {
            _committedChanges.Clear();
            _completed = true;
        }
    }

    public void Dispose()
    {
        Rollback();
    }

    private void RollbackEntries(int count)
    {
        Exception? rollbackError = null;
        for (var index = count - 1; index >= 0; index--)
        {
            try
            {
                _entries[index].Rollback();
            }
            catch (Exception exception)
            {
                rollbackError ??= exception;
            }
        }

        if (rollbackError is not null)
        {
            throw new InvalidOperationException(
                "State transaction rollback failed.",
                rollbackError);
        }
    }
}



