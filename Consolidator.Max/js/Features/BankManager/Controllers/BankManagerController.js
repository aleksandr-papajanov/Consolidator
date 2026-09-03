const { BankManagerSelection } = require("./BankManagerSelection.js");
const { BankManagerTopologyEditor } = require("./BankManagerTopologyEditor.js");
const { BankManagerStateActions } = require("./BankManagerStateActions.js");
const { routeBankManagerIntent } = require("./BankManagerIntentRouter.js");

class BankManagerController
{
    constructor(context)
    {
        this.context = context;
        this.selection = new BankManagerSelection(context);
        this.topology = new BankManagerTopologyEditor(context);
        this.stateActions = new BankManagerStateActions(context);
        this.historyJumpPending = false;
        this.unsubscribeHistory = context.transactions &&
            typeof context.transactions.subscribeHistory === "function"
            ? context.transactions.subscribeHistory(() => {
                if (!this.historyJumpPending) return;
                this.historyJumpPending = false;
                this.refreshHistoryTarget();
            }, false) : null;
    }

    selectBank(instanceId, bankId, extendSelection)
    {
        this.selection.selectBank(instanceId, bankId, extendSelection);
    }

    selectPanel(panel)
    {
        this.selection.selectPanel(panel);
    }

    selectProcessor(instanceId, processorId)
    {
        this.selection.selectProcessor(instanceId, processorId);
    }

    selectRow(instanceId)
    {
        this.selection.selectRow(instanceId);
    }

    toggleBankSelection(instanceId, bankId, extendSelection)
    {
        this.context.viewModel.toggleBankSelection(
            instanceId, bankId, extendSelection);
    }

    groupSelectedBanks()
    {
        this.topology.groupSelectedBanks();
    }

    ungroupFocusedBank()
    {
        this.topology.ungroupFocusedBank();
    }

    clearGroups()
    {
        this.topology.clear();
    }

    handleIntent(name, values)
    {
        routeBankManagerIntent(this, name, values);
    }

    jumpHistory(cursor)
    {
        if (!this.context.transactions) return;
        let target = Number(cursor);
        if (!isFinite(target) || target < 0 ||
                target > this.context.transactions.history.entryCount) {
            return;
        }
        this.historyJumpPending = true;
        this.context.transactions.jumpHistory(target, (response) => {
            if (response && response.error) {
                this.historyJumpPending = false;
                return;
            }
            if (this.historyJumpPending) {
                this.historyJumpPending = false;
                this.refreshHistoryTarget();
            }
        });
    }

    refreshHistoryTarget()
    {
        let targetState = this.context.uiTarget &&
            this.context.uiTarget.targetState;
        let target = targetState && targetState.target;
        if (target) {
            this.context.uiTarget.show(
                target.instanceId, target.bankId, target.snapshotContext);
        }
    }

    setMute(instanceId, value, additive)
    {
        this.stateActions.setInstance(
            "set_instance_mute", instanceId, value, additive);
    }

    setBypass(instanceId, value, additive)
    {
        this.stateActions.setInstance(
            "set_instance_bypass", instanceId, value, additive);
    }

    setSolo(instanceId, value, additive)
    {
        this.stateActions.setInstance(
            "set_instance_solo", instanceId, value, additive);
    }

    resetInstance()
    {
        this.stateActions.resetInstance();
    }

    setProcessorBypass(instanceId, processorId, value)
    {
        this.stateActions.setProcessorBypass(instanceId, processorId, value);
    }

    setBankBypass(instanceId, bankId, value)
    {
        this.stateActions.setBankBypass(instanceId, bankId, value);
    }

    resetProcessor(processorId, instanceId)
    {
        this.stateActions.resetProcessor(processorId, instanceId);
    }

    resetBank(instanceId, bankId)
    {
        this.stateActions.resetBank(instanceId, bankId);
    }

    resolveInstanceControlScope(instanceId, additive)
    {
        return this.stateActions.resolveInstanceScope(instanceId, additive);
    }

    destroy()
    {
        if (this.unsubscribeHistory) this.unsubscribeHistory();
        this.unsubscribeHistory = null;
        this.historyJumpPending = false;
        this.selection = null;
        this.topology = null;
        this.stateActions = null;
        this.context = null;
    }
}

module.exports = {
    BankManagerController: BankManagerController
};
