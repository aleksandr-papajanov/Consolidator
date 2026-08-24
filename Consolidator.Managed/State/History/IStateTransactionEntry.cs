namespace Consolidator.Managed.State.History;

internal interface IStateTransactionEntry
{
    void Commit();

    void Rollback();

    void Complete();
}



