using Consolidator.Managed.State.History;

namespace Consolidator.Managed.Core.Services.Abstractions;

public interface IHistoryStateSink
{
    void Publish(StateHistorySnapshot snapshot);
}