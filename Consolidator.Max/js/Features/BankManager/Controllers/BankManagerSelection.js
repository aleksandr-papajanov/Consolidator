class BankManagerSelection
{
    constructor(context)
    {
        this.context = context;
    }

    selectBank(instanceId, bankId, extendSelection)
    {
        let viewModel = this.context.viewModel;
        if (extendSelection) {
            if (viewModel.canSelectBank(instanceId, bankId, true)) {
                viewModel.toggleBankSelection(instanceId, bankId, true);
            }
            return;
        }

        let focusedChanged = viewModel.setFocusedBank(instanceId, bankId);
        if (viewModel.canSelectBank(instanceId, bankId, false)) {
            viewModel.toggleBankSelection(instanceId, bankId, false);
        }
        if (focusedChanged !== false) {
            this.show(instanceId, bankId, viewModel.selectedPanel);
        }
    }

    selectPanel(panel)
    {
        let target = this.context.uiTarget.targetState.target;
        if (target) this.show(target.instanceId, target.bankId, panel);
    }

    selectProcessor(instanceId, processorId)
    {
        let current = this.context.viewModel.focusedSelection;
        if (!current) return;
        if (String(current.instanceId) === String(instanceId)) {
            this.selectPanel(processorId);
            return;
        }

        let row = this.context.viewModel.rows.filter((candidate) => {
            return String(candidate.instanceId) === String(instanceId);
        })[0];
        let bank = row && row.banks.filter((candidate) => {
            return Number(candidate.bankId) === Number(current.bankId);
        })[0];
        if (!bank) return;

        this.context.viewModel.setFocusedBank(instanceId, bank.bankId);
        this.show(instanceId, bank.bankId, processorId);
    }

    selectRow(instanceId)
    {
        let row = this.context.viewModel.rows.filter((candidate) => {
            return String(candidate.instanceId) === String(instanceId);
        })[0];
        if (row && row.banks.length) {
            this.selectBank(instanceId, row.banks[0].bankId);
        }
    }

    show(instanceId, bankId, panel)
    {
        this.context.uiTarget.show(instanceId, bankId, panel, (response) => {
            if (!response || response.error || !response.snapshotContext) return;
            this.context.viewModel.setSelectedPanel(response.snapshotContext);
            this.context.onSnapshotAccepted(response.snapshotContext);
        });
    }
}

module.exports = {
    BankManagerSelection: BankManagerSelection
};
