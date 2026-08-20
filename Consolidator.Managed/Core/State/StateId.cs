namespace Consolidator.Managed.Core.State;

public readonly record struct StateId(uint Value);

public static class StateIds
{
	public static readonly StateId Gain = new(1);
	public static readonly StateId Topology = new(2);
}
