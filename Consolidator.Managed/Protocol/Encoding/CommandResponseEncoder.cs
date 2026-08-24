using Consolidator.Managed.Core.Commands.Results;
using Consolidator.Managed.Protocol.Messages;

namespace Consolidator.Managed.Protocol.Encoding;

internal sealed class CommandResponseEncoder
{
    public IReadOnlyList<Atom> Encode<TResult>(
        CommandExecutionResult<TResult> result,
        ulong sourceInstanceId,
        ulong requestId)
    {
        var atoms = new List<Atom>
        {
            new(AtomType.Integer, 1, 0, null),
            new(AtomType.Symbol, 0, 0, sourceInstanceId.ToString()),
            new(AtomType.Symbol, 0, 0, requestId.ToString()),
            new(AtomType.Integer, result.Succeeded ? 1 : 0, 0, null),
            new(AtomType.Integer, result.TargetCount, 0, null),
            new(AtomType.Integer, result.AppliedCount, 0, null)
        };

        if (result.Error is not null)
        {
            atoms.Add(new Atom(AtomType.Symbol, 0, 0, result.Error));
        }

        AppendResultValue(atoms, result.Value);

        return atoms;
    }

    private static void AppendResultValue<TResult>(
        ICollection<Atom> atoms,
        TResult? value)
    {
        switch (value)
        {
            case RegistrySnapshotResult registry:
                atoms.Add(new Atom(AtomType.Integer, registry.Instances.Count, 0, null));
                foreach (var instance in registry.Instances)
                {
                    atoms.Add(new Atom(AtomType.Symbol, 0, 0, instance.InstanceId.ToString()));
                    atoms.Add(new Atom(AtomType.Symbol, 0, 0, instance.Label));
                    atoms.Add(new Atom(
                        AtomType.Integer,
                        instance.FocusedBankId is { } focused ? (long)focused + 1 : 0,
                        0,
                        null));
                    atoms.Add(new Atom(AtomType.Integer, instance.Banks.Count, 0, null));
                    foreach (var bank in instance.Banks)
                    {
                        atoms.Add(new Atom(AtomType.Integer, bank.BankId + 1, 0, null));
                        atoms.Add(new Atom(
                            AtomType.Symbol,
                            0,
                            0,
                            bank.GroupId?.ToString() ?? string.Empty));
                    }
                }

                break;
            case null:
                break;
            default:
                atoms.Add(ProtocolAtomEncoder.EncodeValue(value));
                break;
        }
    }

}
