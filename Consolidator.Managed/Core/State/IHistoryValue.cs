namespace Consolidator.Managed.Core.State;

internal interface IHistoryValue
{
    void CopySlot(
        int sourceSlot,
        int destinationSlot);

    void ApplySlot(int slot);
}
