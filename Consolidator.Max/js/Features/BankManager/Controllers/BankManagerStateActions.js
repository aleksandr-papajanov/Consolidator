const ProcessorResetPaths = {
    input: "input_gain",
    saturator: "saturator",
    compressor: "compressor",
    equalizer: "equalizer",
    polish: "polish",
    output: "output_gain"
};

class BankManagerStateActions
{
    constructor(context)
    {
        this.context = context;
    }

    setInstance(selector, instanceId, value, additive)
    {
        this.context.protocol.request(selector, [
            instanceId,
            this.resolveInstanceScope(instanceId, additive),
            value ? 1 : 0,
            additive ? "additive" : "exclusive"
        ]);
    }

    setProcessorBypass(instanceId, processorId, value)
    {
        this.context.protocol.request("set_processor_bypass", [
            instanceId, processorId, this.context.scope.mode, value ? 1 : 0
        ]);
    }

    setBankBypass(instanceId, bankId, value)
    {
        this.context.protocol.request("set_bank_bypass", [
            instanceId, bankId, this.context.scope.mode, value ? 1 : 0
        ]);
    }

    resetInstance()
    {
        let scope = this.context.scope.mode === "group"
            ? "group_instance" : "local";
        this.context.state.reset("dsp", undefined, 0, scope);
    }

    resetProcessor(processorId, instanceId)
    {
        let path = ProcessorResetPaths[String(processorId)];
        if (!path || instanceId === undefined || instanceId === null) return;
        this.context.state.resetTargeted(
            instanceId, null, path, undefined, 0, this.context.scope.mode);
    }

    resetBank(instanceId, bankId)
    {
        if (instanceId === undefined || instanceId === null ||
                bankId === undefined || bankId === null) return;
        this.context.state.resetTargeted(
            instanceId, Number(bankId), "equalizer.bank", undefined,
            0, this.context.scope.mode);
    }

    resolveInstanceScope(instanceId, additive)
    {
        let viewModel = this.context.viewModel;
        let focusedBank = typeof viewModel.focusedBankFor === "function"
            ? viewModel.focusedBankFor(instanceId) : null;
        let grouped = focusedBank && focusedBank.groupId !== undefined &&
            focusedBank.groupId !== null && Number(focusedBank.groupId) >= 0;
        return additive && !grouped ? "local" : this.context.scope.mode;
    }
}

module.exports = {
    BankManagerStateActions: BankManagerStateActions
};
