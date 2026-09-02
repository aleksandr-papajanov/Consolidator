namespace Consolidator.Managed.Core.Exceptions;

internal abstract class ManagedException : Exception
{
    protected ManagedException(string message)
        : base(message)
    {
    }

    protected ManagedException(string message, Exception innerException)
        : base(message, innerException)
    {
    }
}
