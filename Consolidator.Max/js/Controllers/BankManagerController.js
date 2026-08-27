const { BankManagerContext } = require("./BankManagerContext.js");

class BankManagerController
{
    constructor(context)
    {
        this.context = context;
        this.clearConfirmationArmed = false;
        this.clearConfirmationTimer = null;
    }
    
    updateClearAction(armed)
    {
        this.clearConfirmationArmed = armed;
        this.context.viewModel.apply({
            clearAction: {
                enabled: Boolean(this.context.viewModel.clearAction.enabled),
                armed: armed
            }
        });
    }
    
    disarmClearConfirmation()
    {
        this.clearConfirmationTimer = null;
        this.updateClearAction(false);
    }
    
    armClearConfirmation()
    {
        this.updateClearAction(true);
        if (this.clearConfirmationTimer !== null &&
                typeof clearTimeout === "function") {
            clearTimeout(this.clearConfirmationTimer);
        }
        if (typeof setTimeout === "function") {
            this.clearConfirmationTimer = setTimeout(() => {
                this.disarmClearConfirmation();
            }, 3000);
        }
    }
    
    selectBank(
        instanceId,
        bankId,
        extendSelection
    )
    {
        if (this.context.viewModel.linkEditing) {
            this.toggleBankSelection(instanceId, bankId, extendSelection);
            return;
        }
        if (this.context.viewModel.setFocusedBank(instanceId, bankId) === false) {
            return;
        }
        this.context.uiTarget.show(instanceId, bankId);
    }
    
    selectRow(instanceId)
    {
        let row = this.context.viewModel.rows.filter((candidate) => {
            return String(candidate.instanceId) === String(instanceId);
        })[0];
        if (!row || !row.banks.length) return;
        this.selectBank(instanceId, row.banks[0].bankId);
    }
    
    toggleBankSelection(
        instanceId,
        bankId,
        extendSelection
    )
    {
        this.context.viewModel.toggleBankSelection(
            instanceId,
            bankId,
            extendSelection
        );
    }
    
    toggleLinkEditing()
    {
        this.context.viewModel.toggleLinkEditing();
    }
    
    applyLinkGroup(linkId)
    {
        let context = this.context;
        let entriesByInstance = {};
        context.viewModel.getSelectedBanks()
            .forEach((selection) => {
                let instanceId = String(selection.instanceId);
                if (!entriesByInstance[instanceId]) {
                    entriesByInstance[instanceId] = [];
                }
                entriesByInstance[instanceId].push({
                    path: "bank." + selection.bankId + ".group",
                    value: Number(linkId)
                });
            });
        Object.keys(entriesByInstance).forEach((instanceId) => {
            context.state.setManyFor(
                instanceId,
                entriesByInstance[instanceId]
            );
        });
        context.viewModel.clearBankSelection();
        context.viewModel.toggleLinkEditing();
    }
    
    clearLocalGroups()
    {
        if (!this.clearConfirmationArmed) {
            this.armClearConfirmation();
            return;
        }
        this.disarmClearConfirmation();
        let entries = [];
        for (let bankId = 0; bankId < 7; bankId += 1) {
            entries.push({ path: "bank." + bankId + ".group", value: null });
        }
        this.context.state.setManyFor(this.context.instanceId, entries);
    }
    
    handleIntent(name, values)
    {
        values = values || [];
        switch (name) {
        case "bankSelected":
            this.selectBank(values[0], values[1], Number(values[2]) !== 0);
            break;
        case "rowSelected": this.selectRow(values[0]); break;
        case "linkGroupSelected": this.applyLinkGroup(values[0]); break;
        case "editToggled": this.toggleLinkEditing(); break;
        case "clearRequested": this.clearLocalGroups(); break;
        }
    }
    
    destroy()
    {
        if (this.clearConfirmationTimer !== null &&
                typeof clearTimeout === "function") {
            clearTimeout(this.clearConfirmationTimer);
        }
        this.clearConfirmationTimer = null;
        this.context = null;
    }
}

module.exports = {
    BankManagerController: BankManagerController
};

