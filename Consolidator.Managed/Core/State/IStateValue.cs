namespace Consolidator.Managed.Core.State;

internal interface IStateValue : IDisposable
{
    StateId Id { get; }
}