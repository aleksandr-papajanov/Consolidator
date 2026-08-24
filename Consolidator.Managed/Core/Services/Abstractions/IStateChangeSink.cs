using Consolidator.Managed.Core.State;

namespace Consolidator.Managed.Core.Services.Abstractions;

public interface IStateChangeSink
{
    void Publish(StateValueChanged change);
}