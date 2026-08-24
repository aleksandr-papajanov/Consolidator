using Consolidator.Managed.Core.Commands.Results;
using Consolidator.Managed.Core.State;

namespace Consolidator.Managed.Routing.Commands;

public sealed record CommandRoutingResult<TResult>(
    IReadOnlyList<InstanceId> TargetInstanceIds,
    CommandExecutionResult<TResult> Execution);
