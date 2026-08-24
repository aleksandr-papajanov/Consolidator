namespace Consolidator.Managed.State.History;

internal interface IHistoryValue
{
    void SetCurrentSlot(int slot);

    void CopySlot(int sourceSlot, int destinationSlot);

    void ApplySlot(int slot);
}



