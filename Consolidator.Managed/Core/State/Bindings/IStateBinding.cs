namespace Consolidator.Managed.Core.State.Bindings;

public interface IStateBinding<in TValue>
{
    void Apply(TValue value);
}
