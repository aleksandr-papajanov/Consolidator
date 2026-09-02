using Consolidator.Managed.Core.Commands.Results;
using Consolidator.Managed.Core.State;
using Consolidator.Managed.Protocol.Messages;
using Consolidator.Managed.State;

namespace Consolidator.Managed.Protocol.Encoding;

internal sealed class CommandResponseEncoder
{
    public IReadOnlyList<ProtocolOutput> Encode<TResult>(
        string responseSelector,
        CommandExecutionResult<TResult> result,
        ulong sourceInstanceId,
        ulong requestId)
    {
        if (!result.Succeeded)
        {
            return [Output(sourceInstanceId, "error", Header(sourceInstanceId, requestId)
                .Append(Symbol(result.Error ?? "Command failed.")).ToArray())];
        }

        return result.Value switch
        {
            UiInitializationResult initialized =>
                [Output(sourceInstanceId, "initialized", Header(sourceInstanceId, requestId)
                    .Concat([
                        Symbol(initialized.InstanceId.ToString()),
                        Symbol(ProcessorIds.Encode(initialized.SnapshotContext))
                    ]).ToArray())],
            TargetStateSnapshotResult snapshot => EncodeSnapshot(sourceInstanceId, requestId, snapshot),
            RegistrySnapshotResult registry => EncodeRegistry(sourceInstanceId, requestId, registry),
            StateWriteStatus status => [Output(sourceInstanceId, responseSelector,
                Header(sourceInstanceId, requestId)
                    .Append(Integer(status is StateWriteStatus.Applied or StateWriteStatus.Unchanged ? 1 : 0))
                    .ToArray())],
            _ when responseSelector == "state_done" =>
                [Output(sourceInstanceId, responseSelector,
                    Header(sourceInstanceId, requestId)
                        .Append(ProtocolAtomEncoder.EncodeValue(result.Value)).ToArray())],
            _ => [Output(sourceInstanceId, responseSelector,
                Header(sourceInstanceId, requestId).Append(Integer(1)).ToArray())]
        };
    }

    private static IReadOnlyList<ProtocolOutput> EncodeSnapshot(
        ulong target,
        ulong requestId,
        TargetStateSnapshotResult snapshot)
    {
        var atoms = new List<Atom>(6 + snapshot.Values.Count * 6);
        atoms.AddRange(Header(target, requestId));
        atoms.Add(Symbol(snapshot.InstanceId.ToString()));
        atoms.Add(Integer(snapshot.BankId));
        atoms.Add(Symbol(ProcessorIds.Encode(snapshot.SnapshotContext)));
        atoms.Add(Integer(snapshot.Values.Count));
        for (var index = 0; index < snapshot.Values.Count; index++)
        {
            var value = snapshot.Values[index];
            atoms.Add(Symbol(StatePathEncoder.Encode(value.Path, snapshot.BankId)));
            atoms.Add(ProtocolAtomEncoder.EncodeValue(value.Value));
            atoms.Add(Optional(value.PhysicalRange?.Minimum));
            atoms.Add(Optional(value.PhysicalRange?.Maximum));
            atoms.Add(Optional(value.EffectiveRange?.Minimum));
            atoms.Add(Optional(value.EffectiveRange?.Maximum));
        }
        return [Output(target, "target_state_snapshot", atoms)];
    }

    private static IReadOnlyList<ProtocolOutput> EncodeRegistry(
        ulong target,
        ulong requestId,
        RegistrySnapshotResult registry)
    {
        var processorMarkers = registry.ProcessorMarkers.ToDictionary(
            marker => (marker.InstanceId, marker.ProcessorId),
            marker => marker.Active);
        var outputs = new List<ProtocolOutput>
        {
            Output(target, "registry_begin", Header(target, requestId)
                .Concat([Integer((long)registry.Revision), Integer(registry.Instances.Count), Integer(registry.Groups.Count)])
                .ToArray())
        };
        foreach (var instance in registry.Instances)
        {
            outputs.Add(Output(target, "registry_instance", Header(target, requestId)
                .Concat([Symbol(instance.InstanceId.ToString()), Symbol(instance.Label),
                    Integer(instance.Mute ? 1 : 0), Integer(instance.Solo ? 1 : 0),
                    Integer(instance.Bypass ? 1 : 0)]).ToArray()));
            foreach (var processor in instance.Processors)
            {
                var markerActive = processorMarkers[(instance.InstanceId, processor.ProcessorId)];
                outputs.Add(Output(target, "registry_processor", Header(target, requestId)
                    .Concat([Symbol(instance.InstanceId.ToString()),
                        Symbol(ProcessorIds.Encode(processor.ProcessorId)),
                        Integer(processor.EffectActive ? 1 : 0),
                        Integer(markerActive ? 1 : 0),
                        Integer(processor.Bypassed ? 1 : 0)]).ToArray()));
            }
            foreach (var bank in instance.Banks)
            {
                outputs.Add(Output(target, "registry_bank", Header(target, requestId)
                    .Concat([Symbol(instance.InstanceId.ToString()), Integer(bank.BankId),
                        bank.GroupId is { } group ? Integer(group) : Symbol("none"),
                        Integer(bank.EffectActive ? 1 : 0)]).ToArray()));
            }
        }
        foreach (var group in registry.Groups)
        {
            outputs.Add(Output(target, "registry_group", Header(target, requestId)
                .Append(Integer(group.GroupId)).ToArray()));
            foreach (var member in group.Members)
            {
                outputs.Add(Output(target, "registry_member", Header(target, requestId)
                    .Concat([Integer(group.GroupId), Symbol(member.InstanceId.ToString()), Integer(member.BankId)])
                    .ToArray()));
            }
        }
        outputs.Add(Output(target, "registry_done", Header(target, requestId)
            .Append(Integer((long)registry.Revision)).ToArray()));
        return outputs;
    }

    private static IEnumerable<Atom> Header(ulong source, ulong requestId) =>
        [Integer(1), Symbol(source.ToString()), Symbol(requestId.ToString())];

    private static ProtocolOutput Output(ulong target, string selector, IReadOnlyList<Atom> atoms) =>
        new([target], selector, atoms, DeliverySemantics.Lossless);

    private static Atom Optional(float? value) => value is { } number
        ? new Atom(AtomType.Float, 0, number, null)
        : Symbol("none");

    private static Atom Integer(long value) => new(AtomType.Integer, value, 0, null);
    private static Atom Symbol(string value) => new(AtomType.Symbol, 0, 0, value);
}
