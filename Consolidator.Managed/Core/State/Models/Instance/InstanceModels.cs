using Consolidator.Managed.Core.State.Models.Dsp;
using Consolidator.Managed.State;

namespace Consolidator.Managed.Core.State.Models.Instance;

public sealed record BankState(
    BankId Id,
    StateValue<GroupId?> Group,
    StateValue<bool> Bypass,
    EqualizerBankState Equalizer);

public sealed record InstanceState(
    InstanceId InstanceId,
    StateValue<string> Label,
    StateValue<bool> Mute,
    StateValue<bool> Solo,
    StateValue<bool> Bypass,
    BankState[] Banks);