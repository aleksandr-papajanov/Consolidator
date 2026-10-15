using Consolidator.Managed.Core.Commands.Definitions;
using Consolidator.Managed.Core.Settings;
using Consolidator.Managed.Protocol.Dispatch;
using Consolidator.Managed.Protocol.Messages;
using Consolidator.Managed.State;

namespace Consolidator.Managed.Protocol.Decoding;

internal sealed class GroupBanksInputCodec : IInputCodec
{
    public string Selector => "group_banks";

    public DecodedCommand Decode(ReadOnlySpan<Atom> atoms, CommandFrameHeader header)
    {
        if (atoms.Length <= header.Position)
        {
            throw new FormatException("Missing group bank count.");
        }
        var countAtom = atoms[header.Position];
        var position = header.Position + 1;
        if (countAtom.Type != AtomType.Integer || countAtom.Integer < 2 ||
            countAtom.Integer != (atoms.Length - position) / 2 ||
            (atoms.Length - position) % 2 != 0)
        {
            throw new FormatException("Invalid group bank list.");
        }

        var count = (int)countAtom.Integer;
        var banks = new BankAddress[count];
        for (var index = 0; index < count; index++)
        {
            var instance = new InstanceId(CommandCodecSupport.ReadWireId(atoms[position++]));
            var bank = atoms[position++];
            if (bank.Type != AtomType.Integer || bank.Integer is < 0 or >= DspConstants.BankCount)
            {
                throw new FormatException("Bank index is out of range.");
            }
            banks[index] = new BankAddress(instance, (int)bank.Integer);
        }
        return CommandCodecSupport.Success(header, new GroupBanksCommand(banks));
    }
}

internal sealed class UngroupBanksInputCodec : IInputCodec
{
    public string Selector => "ungroup_banks";

    public DecodedCommand Decode(ReadOnlySpan<Atom> atoms, CommandFrameHeader header)
    {
        if (atoms.Length != header.Position + 2)
        {
            throw new FormatException("Invalid ungroup bank frame.");
        }
        var instance = new InstanceId(CommandCodecSupport.ReadWireId(atoms[header.Position]));
        var bank = atoms[header.Position + 1];
        if (bank.Type != AtomType.Integer || bank.Integer is < 0 or >= DspConstants.BankCount)
        {
            throw new FormatException("Bank index is out of range.");
        }
        return CommandCodecSupport.Success(header, new UngroupBanksCommand(instance, (int)bank.Integer));
    }
}
