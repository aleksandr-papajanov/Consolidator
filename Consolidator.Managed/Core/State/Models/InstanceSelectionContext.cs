using Consolidator.Managed.Core.State;

namespace Consolidator.Managed.Core.State.Models;

public readonly record struct InstanceSelectionContext(
    BankAddress? SelectedBank,
    ProcessorId SelectedProcessor);
