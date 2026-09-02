namespace Consolidator.Managed.Core.State.Definitions;

public readonly record struct FloatRange(
    float Minimum,
    float Maximum)
{
    public bool IsValid => Minimum <= Maximum;

    public bool Contains(float value) =>
        IsValid && value >= Minimum && value <= Maximum;
}



