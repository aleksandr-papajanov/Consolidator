using Consolidator.Managed.Core.Commands.Definitions;
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

internal sealed class ClearTopologyInputCodec : IInputCodec
{
    public string Selector => "clear_topology";

    public DecodedCommand Decode(
        ReadOnlySpan<Atom> atoms,
        CommandFrameHeader header)
    {
        if (atoms.Length != header.Position)
        {
            throw new FormatException("Invalid clear_topology frame.");
        }

        return CommandCodecSupport.Success(header, new ClearTopologyCommand());
    }
}

internal sealed class ObserveTargetInputCodec : IInputCodec
{
    public string Selector => "observe_target";

    public DecodedCommand Decode(
        ReadOnlySpan<Atom> atoms,
        CommandFrameHeader header)
    {
        if (atoms.Length != header.Position + 3)
        {
            throw new FormatException("Invalid observe_target frame.");
        }

        var instanceId = CommandCodecSupport.ReadWireId(atoms[header.Position]);
        if (atoms[header.Position + 1].Type != AtomType.Integer ||
            atoms[header.Position + 1].Integer is < 0 or >= 7)
        {
            throw new FormatException("Observed bank is out of range.");
        }

        if (atoms[header.Position + 2].Type != AtomType.Symbol)
        {
            throw new FormatException("Invalid snapshot context.");
        }

        return CommandCodecSupport.Success(
            header,
            new ObserveTargetCommand(
                new InstanceId(instanceId),
                (BankId)atoms[header.Position + 1].Integer,
                ProcessorIds.Parse(atoms[header.Position + 2].Symbol)));
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

internal sealed class SetInstanceMuteInputCodec : IInputCodec
{
    public string Selector => "set_instance_mute";

    public DecodedCommand Decode(
        ReadOnlySpan<Atom> atoms,
        CommandFrameHeader header)
    {
        var targetPosition = header.Position;
        if (targetPosition >= atoms.Length)
        {
            throw new FormatException("Missing target instance id.");
        }

        var targetInstanceId = new InstanceId(
            CommandCodecSupport.ReadWireId(atoms[targetPosition++]));
        var targetScope = InstanceControlInputCodecSupport.ReadScope(
            atoms,
            header with { Position = targetPosition },
            out var valuePosition);
        if (atoms.Length != valuePosition + 2 ||
            atoms[valuePosition].Type != AtomType.Integer ||
            atoms[valuePosition].Integer is < 0 or > 1 ||
            atoms[valuePosition + 1].Type != AtomType.Symbol)
        {
            throw new FormatException("Invalid set_instance_mute frame.");
        }

        var mode = atoms[valuePosition + 1].Symbol switch
        {
            "exclusive" => InstanceControlSelectionMode.Exclusive,
            "additive" => InstanceControlSelectionMode.Additive,
            _ => throw new FormatException("Invalid mute selection mode.")
        };

        return CommandCodecSupport.Success(
            header,
            new SetInstanceMuteCommand(
                targetScope,
                atoms[valuePosition].Integer == 1,
                mode,
                targetInstanceId));
    }
}

internal sealed class SetInstanceSoloInputCodec : IInputCodec
{
    public string Selector => "set_instance_solo";

    public DecodedCommand Decode(
        ReadOnlySpan<Atom> atoms,
        CommandFrameHeader header)
    {
        var targetPosition = header.Position;
        if (targetPosition >= atoms.Length)
        {
            throw new FormatException("Missing target instance id.");
        }

        var targetInstanceId = new InstanceId(
            CommandCodecSupport.ReadWireId(atoms[targetPosition++]));
        var targetScope = InstanceControlInputCodecSupport.ReadScope(
            atoms,
            targetPosition,
            out var valuePosition);
        if (atoms.Length != valuePosition + 2 ||
            atoms[valuePosition].Type != AtomType.Integer ||
            atoms[valuePosition].Integer is < 0 or > 1 ||
            atoms[valuePosition + 1].Type != AtomType.Symbol)
        {
            throw new FormatException("Invalid set_instance_solo frame.");
        }

        var mode = atoms[valuePosition + 1].Symbol switch
        {
            "exclusive" => InstanceControlSelectionMode.Exclusive,
            "additive" => InstanceControlSelectionMode.Additive,
            _ => throw new FormatException("Invalid solo selection mode.")
        };
        return CommandCodecSupport.Success(
            header,
            new SetInstanceSoloCommand(
                targetScope,
                atoms[valuePosition].Integer == 1,
                mode,
                targetInstanceId));
    }
}

internal sealed class SetInstanceBypassInputCodec : IInputCodec
{
    public string Selector => "set_instance_bypass";

    public DecodedCommand Decode(ReadOnlySpan<Atom> atoms, CommandFrameHeader header)
    {
        var targetPosition = header.Position;
        if (targetPosition >= atoms.Length)
        {
            throw new FormatException("Missing target instance id.");
        }

        var targetInstanceId = new InstanceId(
            CommandCodecSupport.ReadWireId(atoms[targetPosition++]));
        var targetScope = InstanceControlInputCodecSupport.ReadScope(
            atoms, targetPosition, out var valuePosition);
        if (atoms.Length != valuePosition + 2 ||
            atoms[valuePosition].Type != AtomType.Integer ||
            atoms[valuePosition].Integer is < 0 or > 1 ||
            atoms[valuePosition + 1].Type != AtomType.Symbol)
        {
            throw new FormatException("Invalid set_instance_bypass frame.");
        }

        var mode = atoms[valuePosition + 1].Symbol switch
        {
            "exclusive" => InstanceControlSelectionMode.Exclusive,
            "additive" => InstanceControlSelectionMode.Additive,
            _ => throw new FormatException("Invalid bypass selection mode.")
        };
        return CommandCodecSupport.Success(
            header,
            new SetInstanceBypassCommand(
                targetScope,
                atoms[valuePosition].Integer == 1,
                mode,
                targetInstanceId));
    }
}

internal sealed class SetProcessorBypassInputCodec : IInputCodec
{
    public string Selector => "set_processor_bypass";

    public DecodedCommand Decode(ReadOnlySpan<Atom> atoms, CommandFrameHeader header)
    {
        if (atoms.Length < header.Position + 4)
        {
            throw new FormatException("Invalid set_processor_bypass frame.");
        }

        var targetInstanceId = new InstanceId(
            CommandCodecSupport.ReadWireId(atoms[header.Position]));
        var processor = ProcessorControlInputCodecSupport.ReadProcessor(
            atoms,
            header with { Position = header.Position + 1 });
        var targetScope = InstanceControlInputCodecSupport.ReadScope(
            atoms, header.Position + 2, out var valuePosition);
        if (atoms.Length != valuePosition + 1 ||
            atoms[valuePosition].Type != AtomType.Integer ||
            atoms[valuePosition].Integer is < 0 or > 1)
        {
            throw new FormatException("Invalid set_processor_bypass frame.");
        }

        return CommandCodecSupport.Success(header, new SetProcessorBypassCommand(
            processor, targetScope, atoms[valuePosition].Integer == 1, targetInstanceId));
    }
}

internal sealed class SetBankBypassInputCodec : IInputCodec
{
    public string Selector => "set_bank_bypass";

    public DecodedCommand Decode(ReadOnlySpan<Atom> atoms, CommandFrameHeader header)
    {
        if (atoms.Length < header.Position + 4)
        {
            throw new FormatException("Invalid set_bank_bypass frame.");
        }

        var targetInstanceId = new InstanceId(
            CommandCodecSupport.ReadWireId(atoms[header.Position]));
        var bankIndexPosition = header.Position + 1;
        if (atoms[bankIndexPosition].Type != AtomType.Integer ||
            atoms[bankIndexPosition].Integer is < 0 or >= 7)
        {
            throw new FormatException("Bank index is out of range.");
        }
        var targetScope = InstanceControlInputCodecSupport.ReadScope(
            atoms, bankIndexPosition + 1, out var valuePosition);
        if (atoms.Length != valuePosition + 1 ||
            atoms[valuePosition].Type != AtomType.Integer ||
            atoms[valuePosition].Integer is < 0 or > 1)
        {
            throw new FormatException("Invalid set_bank_bypass frame.");
        }

        return CommandCodecSupport.Success(header, new SetBankBypassCommand(
            targetScope,
            atoms[valuePosition].Integer == 1,
            targetInstanceId,
            (int)atoms[bankIndexPosition].Integer));
    }
}

internal static class ProcessorControlInputCodecSupport
{
    public static ProcessorId ReadProcessor(
        ReadOnlySpan<Atom> atoms,
        CommandFrameHeader header)
    {
        if (atoms.Length <= header.Position || atoms[header.Position].Type != AtomType.Symbol)
        {
            throw new FormatException("Invalid processor ID.");
        }

        return ProcessorIds.Parse(atoms[header.Position].Symbol);
    }
}

internal static class InstanceControlInputCodecSupport
{
    public static InstanceControlScope ReadScope(
        ReadOnlySpan<Atom> atoms,
        CommandFrameHeader header,
        out int valuePosition)
    {
        return ReadScope(atoms, header.Position, out valuePosition);
    }

    public static InstanceControlScope ReadScope(
        ReadOnlySpan<Atom> atoms,
        int position,
        out int valuePosition)
    {
        if (atoms.Length < position + 2 ||
            atoms[position].Type != AtomType.Symbol)
        {
            throw new FormatException("Invalid instance control frame.");
        }

        valuePosition = position + 1;
        if (atoms[position].Symbol == "local")
        {
            return InstanceControlScope.Instance;
        }

        if (atoms[position].Symbol != "group")
        {
            throw new FormatException("Invalid instance control group target.");
        }

        return InstanceControlScope.BankGroup;
    }
}
