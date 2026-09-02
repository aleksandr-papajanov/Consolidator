using Consolidator.Managed.Core.Exceptions;

namespace Consolidator.Managed.Protocol;

internal abstract class ProtocolException : ManagedException
{
    protected ProtocolException(string message)
        : base(message)
    {
    }

    protected ProtocolException(string message, Exception innerException)
        : base(message, innerException)
    {
    }
}
