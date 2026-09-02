namespace Consolidator.Managed.Core.Services.Abstractions;

public interface IStateChangeSink
{
    void Publish(StateValueChanged change);
}