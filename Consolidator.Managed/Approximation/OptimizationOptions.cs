namespace Consolidator.Managed.Approximation;

internal sealed record OptimizationOptions(
    double FunctionTolerance,
    int MaximumEvaluations,
    int MaximumFrequencyPoints)
{
    public static OptimizationOptions Fast { get; } =
        new(1.0e-4, 1000, 128);

    public static OptimizationOptions Balanced { get; } =
        new(1.0e-5, 2000, 256);

    public static OptimizationOptions Precise { get; } =
        new(1.0e-6, 5000, 512);

    public static OptimizationOptions Commit { get; } =
        new(1.0e-9, 10000, 0);
}
