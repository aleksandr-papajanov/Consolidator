using Consolidator.Managed.Core.Commands.Definitions;
using Consolidator.Managed.Core.State;
using Consolidator.Managed.Protocol.Dispatch;
using Consolidator.Managed.Protocol.Messages;

namespace Consolidator.Managed.Protocol.Decoding;

internal sealed class InitializeInputCodec : IInputCodec
{
    public string Selector => "initialize";

    public DecodedCommand Decode(
        ReadOnlySpan<Atom> atoms,
        CommandFrameHeader header)
    {
        if (atoms.Length != header.Position)
        {
            throw new FormatException("Invalid initialize frame.");
        }

        return CommandCodecSupport.Success(header, new InitializeUiCommand());
    }
}

internal sealed class ObserveTargetInputCodec : IInputCodec
{
    public string Selector => "observe_target";

    public DecodedCommand Decode(
        ReadOnlySpan<Atom> atoms,
        CommandFrameHeader header)
    {
        if (atoms.Length != header.Position + 2)
        {
            throw new FormatException("Invalid observe_target frame.");
        }

        var instanceId = CommandCodecSupport.ReadWireId(atoms[header.Position]);
        if (atoms[header.Position + 1].Type != AtomType.Integer ||
            atoms[header.Position + 1].Integer is < 1 or > 7)
        {
            throw new FormatException("Observed bank is out of range.");
        }

        return CommandCodecSupport.Success(
            header,
            new ObserveTargetCommand(
                new InstanceId(instanceId),
                (BankId)(atoms[header.Position + 1].Integer - 1)));
    }
}

internal sealed class SetInstanceActiveInputCodec : IInputCodec
{
    public string Selector => "set_instance_active";

    public DecodedCommand Decode(
        ReadOnlySpan<Atom> atoms,
        CommandFrameHeader header)
    {
        if (atoms.Length != header.Position + 1 ||
            atoms[header.Position].Type != AtomType.Integer ||
            atoms[header.Position].Integer is < 0 or > 1)
        {
            throw new FormatException("Invalid set_instance_active frame.");
        }

        return CommandCodecSupport.Success(
            header,
            new SetInstanceActiveCommand(
                atoms[header.Position].Integer == 1));
    }
}
