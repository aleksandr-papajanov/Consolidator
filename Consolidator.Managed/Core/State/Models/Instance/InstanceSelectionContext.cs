namespace Consolidator.Managed.Core.State.Models.Instance;

public readonly record struct InstanceSelectionContext(
    BankAddress? SelectedBank,
    ProcessorId SelectedProcessor);
