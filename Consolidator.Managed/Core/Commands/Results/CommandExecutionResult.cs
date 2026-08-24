namespace Consolidator.Managed.Core.Commands.Results;

public sealed record CommandExecutionResult<TResult>(
    bool Succeeded,
    TResult? Value,
    int TargetCount,
    int AppliedCount,
    string? Error)
{
    public static CommandExecutionResult<TResult> Success(
        TResult? value,
        int targetCount,
        int appliedCount)
    {
        return new CommandExecutionResult<TResult>(
            true,
            value,
            targetCount,
            appliedCount,
            null);
    }

    public static CommandExecutionResult<TResult> Failure(
        int targetCount,
        int appliedCount,
        string error)
    {
        return new CommandExecutionResult<TResult>(
            false,
            default,
            targetCount,
            appliedCount,
            error);
    }
}



