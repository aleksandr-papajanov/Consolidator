class BankManagerTopologyEditor
{
    constructor(context)
    {
        this.context = context;
    }

    groupSelectedBanks()
    {
        let selected = this.context.viewModel.getSelectedBanks();
        if (selected.length < 2) return;
        let groupId = this.context.viewModel.nextGroupId();
        if (groupId < 0) return;

        let entries = {};
        selected.forEach((selection) => {
            this.add(entries, selection.instanceId, selection.bankId, groupId);
        });
        this.write(entries);
        this.context.viewModel.clearBankSelection();
    }

    ungroupFocusedBank()
    {
        let viewModel = this.context.viewModel;
        let focusedBank = viewModel.focusedBank();
        if (!focusedBank || focusedBank.groupId === null ||
                focusedBank.groupId === undefined ||
                Number(focusedBank.groupId) < 0) {
            return;
        }

        let groupId = Number(focusedBank.groupId);
        let entries = {};
        viewModel.rows.forEach((row) => {
            row.banks.forEach((bank) => {
                if (Number(bank.groupId) === groupId) {
                    this.add(entries, row.instanceId, bank.bankId, null);
                }
            });
        });
        this.write(entries);
        viewModel.clearBankSelection();
    }

    add(entries, instanceId, bankId, value)
    {
        let key = String(instanceId);
        if (!entries[key]) entries[key] = [];
        entries[key].push({
            path: "bank." + bankId + ".group",
            value: value
        });
    }

    write(entriesByInstance)
    {
        this.context.transactions.begin((historyId) => {
            Object.keys(entriesByInstance).forEach((instanceId) => {
                let entries = entriesByInstance[instanceId];
                for (let offset = 0; offset < entries.length; offset += 16) {
                    this.context.state.setManyTopologyFor(
                        instanceId,
                        entries.slice(offset, offset + 16)
                    );
                }
            });
            this.context.transactions.end(historyId);
        });
    }

    clear()
    {
        this.context.transactions.begin((historyId) => {
            this.context.protocol.request("clear_topology", [], () => {
                this.context.transactions.end(historyId);
            });
        });
    }
}

module.exports = {
    BankManagerTopologyEditor: BankManagerTopologyEditor
};
