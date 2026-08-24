using Consolidator.Managed.Core.Services.Abstractions;

namespace Consolidator.Managed.Core.Services;

public sealed class OperationGate : IOperationGate, IDisposable
{
    private readonly SemaphoreSlim _semaphore = new(1, 1);

    public IDisposable Enter()
    {
        _semaphore.Wait();
        return new Releaser(_semaphore);
    }

    public async ValueTask<IDisposable> EnterAsync(
        CancellationToken cancellationToken = default)
    {
        await _semaphore.WaitAsync(cancellationToken);
        return new Releaser(_semaphore);
    }

    public void Dispose()
    {
        _semaphore.Dispose();
    }

    private sealed class Releaser : IDisposable
    {
        private readonly SemaphoreSlim _semaphore;

        public Releaser(SemaphoreSlim semaphore)
        {
            _semaphore = semaphore;
        }

        public void Dispose()
        {
            _semaphore.Release();
        }
    }
}




