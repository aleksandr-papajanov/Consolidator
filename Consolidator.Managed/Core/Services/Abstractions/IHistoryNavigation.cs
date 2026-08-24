namespace Consolidator.Managed.Core.Services.Abstractions;

public interface IHistoryNavigation
{
    void AdvanceHistoryPoint();

    bool JumpToHistory(int cursor);
}




