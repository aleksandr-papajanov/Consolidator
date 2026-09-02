namespace Consolidator.Managed.Core.State.Identifiers;

public readonly record struct InstanceId(ulong Value)
{
    public bool IsValid => Value != 0;

    public override string ToString()
    {
        return Value.ToString();
    }
}




