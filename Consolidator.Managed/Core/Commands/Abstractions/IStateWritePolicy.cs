using Consolidator.Managed.Core.Commands.Definitions;
using Consolidator.Managed.State;

namespace Consolidator.Managed.Core.Commands.Abstractions;

public interface IStateWritePolicy
{
    bool Applies(StatePath path);

    bool IsAllowed(
        WriteStateCommand command,
        InstanceCommandContext context);
}
