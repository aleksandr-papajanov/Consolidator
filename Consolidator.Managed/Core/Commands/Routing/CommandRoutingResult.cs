using Consolidator.Managed.Core.Commands.Results;

namespace Consolidator.Managed.Core.Commands.Routing;

public sealed record CommandRoutingResult<TResult>(
    IReadOnlyList<InstanceId> TargetInstanceIds,
    CommandExecutionResult<TResult> Execution);