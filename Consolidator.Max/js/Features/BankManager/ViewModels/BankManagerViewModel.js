const {
    createBankManagerRow,
    retainSelectableBanks,
    selectionKey
} = require("./BankManagerRows.js");
const {
    deriveBankManagerActions,
    nextBankGroupId
} = require("./BankManagerActions.js");
const {
    applyBankManagerRegistryDelta
} = require("./BankManagerRegistryDelta.js");

class BankManagerViewModel
{
    constructor(registryClient, localInstanceId, historyClient, scope, targetState)
    {
        this.registryClient = registryClient;
        this.localInstanceId = localInstanceId;
        this.scope = scope;
        this.targetState = targetState;
        this.focusedBankBypassed = false;
        this.enabled = true;
        this.selectedPanel = "equalizer";
        this.rows = [];
        this.selectedBanks = {};
        this.focusedSelection = null;
        this.groupAction = { enabled: false, active: false };
        this.ungroupAction = { enabled: false, active: false };
        this.scopeAction = { enabled: false, active: false, color: null };
        this.clearAction = { enabled: false };
        this.listeners = [];
        this.unsubscribeRegistry = null;
        this.registryActive = false;
        this.history = historyClient ? historyClient.history : {
            cursor: 0,
            entryCount: 0,
            canUndo: false,
            canRedo: false
        };
        this.unsubscribeHistory = historyClient
            ? historyClient.subscribeHistory((history) => {
                this.history = history;
                this.notify();
            }, true)
            : null;
        this.unsubscribeTarget = targetState && targetState.subscribe
            ? targetState.subscribe("equalizer.bank.bypass", (entry) => {
                this.focusedBankBypassed = Boolean(entry.value);
                this.notify({ selector: "bank_bypass_changed" });
            }, true) : null;
    }
    
    setRegistryActive(active, callback)
    {
        let next = Boolean(active);
        if (this.registryActive === next) {
            if (next && callback) callback(this.registryClient.get(), { error: null });
            return;
        }
        this.registryActive = next;
        if (this.unsubscribeRegistry) {
            this.unsubscribeRegistry();
            this.unsubscribeRegistry = null;
        }
        if (!next) {
            if (this.scope) {
                this.scope.setGroupContext(false, null);
            }
            this.scopeAction = { enabled: false, active: false, color: null };
            this.notify();
            if (callback) callback(null, { error: null });
            return;
        }
        this.unsubscribeRegistry = this.registryClient.subscribe(
            (snapshot, delta) => {
                this.applyRegistryUpdate(snapshot, delta);
            }, true);
        this.registryClient.fetch(callback);
    }
    
    applyRegistryUpdate(snapshot, delta)
    {
        if (delta && this.applyRegistryDelta(snapshot, delta)) return;
        this.applyRegistrySnapshot(snapshot);
    }
    
    applyRegistryDelta(snapshot, delta)
    {
        if (!applyBankManagerRegistryDelta(this, snapshot, delta)) {
            return false;
        }
        this.refreshActions();
        this.notify(delta);
        return true;
    }
    
    findRowIndex(instanceId)
    {
        for (let index = 0; index < this.rows.length; index += 1) {
            if (String(this.rows[index].instanceId) === String(instanceId)) return index;
        }
        return -1;
    }
    
    createRow(instance)
    {
        return createBankManagerRow(
            instance,
            this.localInstanceId,
            this.selectedBanks,
            this.focusedSelection
        );
    }
    
    applyRegistrySnapshot(snapshot)
    {
        if (!snapshot) return;
        this.selectedBanks = retainSelectableBanks(snapshot, this.selectedBanks);
        this.rows = snapshot.instances.map((instance) => this.createRow(instance));
        this.refreshActions();
        this.notify();
    }

    refreshActions()
    {
        const actions = deriveBankManagerActions(
            this.rows,
            this.getSelectedBanks().length,
            this.focusedBank(),
            this.scope
        );
        this.groupAction = actions.groupAction;
        this.ungroupAction = actions.ungroupAction;
        this.scopeAction = actions.scopeAction;
        this.clearAction = actions.clearAction;
    }
    
    setLocalInstanceId(instanceId)
    {
        if (String(this.localInstanceId) === String(instanceId)) return;
        this.localInstanceId = instanceId;
        this.applyRegistrySnapshot(this.registryClient.get());
    }

    setSelectedPanel(panel)
    {
        this.selectedPanel = String(panel);
        this.notify();
    }

    toggleScope()
    {
        if (!this.scope) return;
        this.scope.toggle();
        this.refreshActions();
        this.notify();
    }
    
    setFocusedBank(instanceId, bankId)
    {
        let nextInstanceId = String(instanceId);
        let nextBankId = Number(bankId);
        let previous = this.focusedSelection;
        if (previous && String(previous.instanceId) === nextInstanceId &&
                Number(previous.bankId) === nextBankId) {
            this.selectedBanks = {};
            this.rows.forEach((row) => {
                row.banks.forEach((bank) => {
                    bank.selected = false;
                });
            });
            this.refreshActions();
            this.notify();
            return false;
        }
        this.selectedBanks = {};
        this.rows.forEach((row) => {
            row.banks.forEach((bank) => {
                bank.selected = false;
            });
        });
    
        let previousRowIndex = previous
            ? this.findRowIndex(previous.instanceId) : -1;
        if (previousRowIndex >= 0) {
            this.rows[previousRowIndex].banks.forEach((bank) => {
                if (Number(bank.bankId) === Number(previous.bankId)) {
                    bank.active = false;
                    bank.focused = false;
                }
            });
        }
        this.focusedSelection = {
            instanceId: instanceId,
            bankId: nextBankId
        };
        this.focusedBankBypassed = false;
        if (this.rows.length === 0) {
            let snapshot = this.registryClient.get();
            if (snapshot) this.applyRegistrySnapshot(snapshot);
            else {
                this.refreshActions();
                this.notify();
            }
            return true;
        }
        let rowIndex = this.findRowIndex(nextInstanceId);
        if (rowIndex >= 0) {
            this.rows[rowIndex].banks.forEach((bank) => {
                if (Number(bank.bankId) === nextBankId) {
                    bank.active = true;
                    bank.focused = true;
                }
            });
        }
        if (this.rows.length > 0) {
            this.refreshActions();
            this.notify({
                selector: "bank_focus_changed",
                rowIndex: rowIndex,
                bankId: nextBankId,
                previousRowIndex: previousRowIndex,
                previousBankId: previous ? Number(previous.bankId) : -1
            });
        }
        return true;
    }
    
    toggleBankSelection(
        instanceId,
        bankId,
        extendSelection
    )
    {
        if (!this.canSelectBank(instanceId, bankId, extendSelection)) {
            return;
        }
        if (!extendSelection) {
            this.selectedBanks = {};
        }
        let key = selectionKey(instanceId, bankId);
        if (this.selectedBanks[key]) delete this.selectedBanks[key];
        else this.selectedBanks[key] = {
            instanceId: instanceId,
            bankId: Number(bankId)
        };
        this.applyRegistrySnapshot(this.registryClient.get());
    }

    focusedBank()
    {
        if (!this.focusedSelection) {
            return null;
        }

        let row = this.rows.filter((candidate) => {
            return String(candidate.instanceId) ===
                String(this.focusedSelection.instanceId);
        })[0];
        if (!row) {
            return null;
        }

        return row.banks.filter((bank) => {
            return Number(bank.bankId) === Number(this.focusedSelection.bankId);
        })[0] || null;
    }

    focusedBankFor(instanceId)
    {
        if (!this.focusedSelection ||
                String(this.focusedSelection.instanceId) !== String(instanceId)) {
            return null;
        }
        return this.focusedBank();
    }

    nextGroupId()
    {
        return nextBankGroupId(this.rows);
    }
    
    getSelectedBanks()
    {
        return Object.keys(this.selectedBanks).map((key) => {
            return this.selectedBanks[key];
        }, this);
    }

    canSelectBank(instanceId, bankId, extendSelection)
    {
        let row = this.rows.filter((candidate) => {
            return String(candidate.instanceId) === String(instanceId);
        })[0];
        if (!row) return false;
        let bank = row.banks.filter((candidate) => {
            return Number(candidate.bankId) === Number(bankId);
        })[0];
        if (!bank || (bank.groupId !== undefined && bank.groupId !== null)) {
            return false;
        }
        if (!extendSelection) return true;
        let key = selectionKey(instanceId, bankId);
        if (this.selectedBanks[key]) return true;
        return !this.getSelectedBanks().some((selection) => {
            return String(selection.instanceId) === String(instanceId);
        });
    }
    
    clearBankSelection()
    {
        this.selectedBanks = {};
        this.applyRegistrySnapshot(this.registryClient.get());
    }
    
    subscribe(callback, immediate)
    {
        this.listeners.push(callback);
        if (immediate) callback(this);
        return () => {
            this.listeners = this.listeners.filter((listener) => {
                return listener !== callback;
            });
        };
    }
    
    notify(delta)
    {
        let listeners = this.listeners.slice();
        for (let index = 0; index < listeners.length; index += 1) {
            listeners[index](this, delta || null);
        }
    }
    
    apply(state)
    {
        state = state || {};
        if (state.enabled !== undefined) this.enabled = Boolean(state.enabled);
        if (state.groupAction !== undefined) this.groupAction = state.groupAction;
        if (state.ungroupAction !== undefined) this.ungroupAction = state.ungroupAction;
        if (state.scopeAction !== undefined) this.scopeAction = state.scopeAction;
        if (state.clearAction !== undefined) this.clearAction = state.clearAction;
        this.notify();
    }
    
    destroy()
    {
        if (this.unsubscribeRegistry) this.unsubscribeRegistry();
        if (this.unsubscribeHistory) this.unsubscribeHistory();
        if (this.unsubscribeTarget) this.unsubscribeTarget();
        this.unsubscribeRegistry = null;
        this.unsubscribeHistory = null;
        this.unsubscribeTarget = null;
        this.listeners = [];
    }
}


module.exports = {
    BankManagerViewModel: BankManagerViewModel
};
