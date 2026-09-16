using Consolidator.Managed.Core.Settings;

namespace Consolidator.Managed.Approximation;

internal sealed record RefinementCandidate(
    IReadOnlyList<int> FilterIndices,
    IReadOnlyList<SolverParameter> Parameters,
    IReadOnlyList<double> Values,
    double Score,
    double ResponseLoss,
    IReadOnlyList<double> Response);

internal readonly record struct SolverParameter(
    double DefaultValue,
    double Minimum,
    double Maximum,
    bool UseLogarithmicScale)
{
    public static SolverParameter From(
        FilterParameterDefinition definition,
        bool useLogarithmicScale = false)
    {
        var range = definition.Range;
        return new SolverParameter(
            definition.DefaultValue,
            range.Minimum,
            range.Maximum,
            useLogarithmicScale);
    }

    public double Normalize(double value)
    {
        if (!UseLogarithmicScale)
        {
            return (value - Minimum) / (Maximum - Minimum);
        }

        return (Math.Log(value) - Math.Log(Minimum)) /
            (Math.Log(Maximum) - Math.Log(Minimum));
    }

    public double Denormalize(double value)
    {
        if (!UseLogarithmicScale)
        {
            return Minimum + value * (Maximum - Minimum);
        }

        return Math.Exp(
            Math.Log(Minimum) +
            value * (Math.Log(Maximum) - Math.Log(Minimum)));
    }
}
