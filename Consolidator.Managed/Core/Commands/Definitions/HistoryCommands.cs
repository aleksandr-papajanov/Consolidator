using Consolidator.Managed.Core.Commands.Abstractions;
using Consolidator.Managed.Core.Commands.Results;

namespace Consolidator.Managed.Core.Commands.Definitions;

public sealed record BeginHistoryCommand(
    ulong HistoryId) : IInstanceCommand<CommandAcknowledgement>
{
    public CommandScope Scope => CommandScope.Coordinator;
}

public sealed record EndHistoryCommand(
    ulong HistoryId) : IInstanceCommand<CommandAcknowledgement>
{
    public CommandScope Scope => CommandScope.Coordinator;
}

public sealed record JumpToHistoryCommand(
    int Cursor) : IInstanceCommand<CommandAcknowledgement>
{
    public CommandScope Scope => CommandScope.Coordinator;
}
