namespace Consolidator.Managed.Core.Services.Abstractions;

public interface IOperationGate
{
    IDisposable Enter();

    ValueTask<IDisposable> EnterAsync(
        CancellationToken cancellationToken = default);
}




