namespace Consolidator.Managed.Protocol;

internal sealed class ProtocolOverloadedException : ProtocolException
{
	public ProtocolOverloadedException()
		: base("The protocol command queue is overloaded.")
	{
	}
}