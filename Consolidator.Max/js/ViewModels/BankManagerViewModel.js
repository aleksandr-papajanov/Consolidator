let BANK_MANAGER_EDITABLE_GROUP_COUNT = 16;
let BANK_MANAGER_GROUP_COLORS = [
    [0.4353, 0.8471, 0.7373, 1],
    [0.4392, 0.7686, 0.7725, 1],
    [0.4392, 0.6902, 0.8039, 1],
    [0.4392, 0.6078, 0.8275, 1],
    [0.4353, 0.5216, 0.8510, 1],
    [0.4824, 0.4902, 0.8196, 1],
    [0.5725, 0.5216, 0.7412, 1],
    [0.6667, 0.5412, 0.6549, 1],
    [0.7569, 0.5569, 0.5608, 1],
    [0.8471, 0.5647, 0.4510, 1],
    [0.9020, 0.6500, 0.4000, 1],
    [0.8500, 0.7600, 0.3300, 1],
    [0.6600, 0.8100, 0.3600, 1],
    [0.3900, 0.8200, 0.5100, 1],
    [0.3000, 0.7700, 0.6900, 1],
    [0.8100, 0.4500, 0.7200, 1]
];

function bankManagerGroupColor(groupId) {
    let index = Number(groupId) % BANK_MANAGER_GROUP_COLORS.length;
    if (index < 0) index += BANK_MANAGER_GROUP_COLORS.length;
    return BANK_MANAGER_GROUP_COLORS[index];
}

function bankManagerGroupIds(snapshotGroups) {
    let groupsById = {};
    (snapshotGroups || []).forEach((group) => {
        groupsById[String(group.groupId)] = group;
    });

    let groups = [];
    for (let groupId = 0;
            groupId < BANK_MANAGER_EDITABLE_GROUP_COUNT;
            groupId += 1) {
        groups.push(groupsById[String(groupId)] || {
            groupId: groupId,
            members: []
        });
    }

    // Keep protocol-valid groups outside the UI's editable range visible.
    (snapshotGroups || []).forEach((group) => {
        if (Number(group.groupId) < 0 ||
                Number(group.groupId) >= BANK_MANAGER_EDITABLE_GROUP_COUNT) {
            groups.unshift(group);
        }
    });
    return groups;
}

class BankManagerViewModel
{
    constructor(registryClient, localInstanceId)
    {
        this.registryClient = registryClient;
        this.localInstanceId = localInstanceId;
        this.enabled = true;
        this.rows = [];
        this.linkEditing = false;
        this.selectedBanks = {};
        this.focusedSelection = null;
        this.linkGroups = [];
        this.editAction = { enabled: false, active: false };
        this.clearAction = { enabled: false, armed: false };
        this.listeners = [];
        this.unsubscribeRegistry = null;
        this.registryActive = false;
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
        let args = delta.args;
        let instanceId = String(args[3]);
        let rowIndex;
        if (delta.selector === "registry_instance_added") {
            let added = snapshot.instances.filter((instance) => {
                return String(instance.instanceId) === instanceId;
            })[0];
            if (!added) return false;
            this.rows.push(this.createRow(added));
            delta.rowIndex = this.rows.length - 1;
        } else if (delta.selector === "registry_instance_removed") {
            rowIndex = this.findRowIndex(instanceId);
            if (rowIndex >= 0) {
                delta.rowIndex = rowIndex;
                this.rows.splice(rowIndex, 1);
            }
        } else if (delta.selector === "registry_label_changed") {
            rowIndex = this.findRowIndex(instanceId);
            if (rowIndex < 0) return false;
            delta.rowIndex = rowIndex;
            this.rows[rowIndex].label = String(args[4]);
        } else if (delta.selector === "registry_bank_group_changed") {
            rowIndex = this.findRowIndex(instanceId);
            if (rowIndex < 0) return false;
            delta.rowIndex = rowIndex;
            let bankId = Number(args[4]);
            this.rows[rowIndex].banks.forEach((bank) => {
                if (bank.bankId === bankId) {
                    bank.groupId = args[5] === "none" ? null : args[5];
                    bank.color = bank.groupId === null ? null : bankManagerGroupColor(bank.groupId);
                }
            });
        } else return false;
        this.updateLinkGroups(snapshot.groups);
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
        let selectedBanks = this.selectedBanks;
        let focusedSelection = this.focusedSelection;
        return {
            instanceId: instance.instanceId,
            label: instance.label,
            local: String(instance.instanceId) === String(this.localInstanceId),
            banks: instance.banks.map((bank) => {
                let bankId = Number(bank.bankId);
                return {
                    bankId: bankId,
                    label: String(bankId),
                    system: false,
                    visible: true,
                    enabled: true,
                    active: Boolean(focusedSelection && String(focusedSelection.instanceId) === String(instance.instanceId) && bankId === focusedSelection.bankId),
                    focused: Boolean(focusedSelection && String(focusedSelection.instanceId) === String(instance.instanceId) && bankId === focusedSelection.bankId),
                    linkSelected: Boolean(selectedBanks[String(instance.instanceId) + ":" + String(bankId)]),
                    groupId: bank.groupId,
                    color: bank.groupId === undefined || bank.groupId === null ? null : bankManagerGroupColor(bank.groupId),
                    opacity: 1
                };
            })
        };
    }
    
    applyRegistrySnapshot(snapshot)
    {
        if (!snapshot) return;
        let selectedBanks = this.selectedBanks;
        let focusedSelection = this.focusedSelection;
        this.rows = snapshot.instances.map((instance) => {
            return {
                instanceId: instance.instanceId,
                label: instance.label,
                local: String(instance.instanceId) === String(this.localInstanceId),
                banks: instance.banks.map((bank) => {
                    let bankId = Number(bank.bankId);
                    return {
                        bankId: bankId,
                        label: String(bankId),
                        system: false,
                        visible: true,
                        enabled: true,
                        active: Boolean(focusedSelection &&
                            String(focusedSelection.instanceId) ===
                                String(instance.instanceId) &&
                            bankId === focusedSelection.bankId),
                        focused: Boolean(focusedSelection &&
                            String(focusedSelection.instanceId) ===
                                String(instance.instanceId) &&
                            bankId === focusedSelection.bankId),
                        linkSelected: Boolean(selectedBanks[
                            String(instance.instanceId) + ":" + String(bankId)
                        ]),
                        groupId: bank.groupId,
                        color: bank.groupId === undefined || bank.groupId === null
                            ? null : bankManagerGroupColor(bank.groupId),
                        opacity: 1
                    };
                })
            };
        }, this);
        this.updateLinkGroups(snapshot.groups);
        this.refreshActions();
        this.notify();
    }
    
    updateLinkGroups(snapshotGroups)
    {
        this.linkGroups = bankManagerGroupIds(snapshotGroups).map((group) => {
            return {
                linkId: group.groupId,
                label: String(group.groupId),
                active: false,
                used: (group.members || []).length > 0,
                enabled: true,
                members: group.members || [],
                color: bankManagerGroupColor(group.groupId)
            };
        });
    }
    
    refreshActions()
    {
        let localRow = this.rows.filter((row) => { return row.local; })[0];
        let editableBanks = localRow ? localRow.banks.filter((bank) => {
            return !bank.system;
        }) : [];
        let editEnabled = editableBanks.length > 0;
        let clearEnabled = editableBanks.some((bank) => {
            return bank.groupId !== undefined && bank.groupId !== null;
        });
        this.editAction = {
            enabled: editEnabled,
            active: editEnabled && this.linkEditing
        };
        this.clearAction = {
            enabled: clearEnabled,
            armed: clearEnabled && Boolean(this.clearAction.armed)
        };
    }
    
    setLocalInstanceId(instanceId)
    {
        if (String(this.localInstanceId) === String(instanceId)) return;
        this.localInstanceId = instanceId;
        this.applyRegistrySnapshot(this.registryClient.get());
    }
    
    setFocusedBank(instanceId, bankId)
    {
        let nextInstanceId = String(instanceId);
        let nextBankId = Number(bankId);
        let previous = this.focusedSelection;
        if (previous && String(previous.instanceId) === nextInstanceId &&
                Number(previous.bankId) === nextBankId) {
            return false;
        }
    
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
        if (this.rows.length === 0) {
            let snapshot = this.registryClient.get();
            if (snapshot) this.applyRegistrySnapshot(snapshot);
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
        if (!extendSelection) {
            this.selectedBanks = {};
        }
        let key = String(instanceId) + ":" + String(bankId);
        if (this.selectedBanks[key]) delete this.selectedBanks[key];
        else this.selectedBanks[key] = {
            instanceId: instanceId,
            bankId: Number(bankId)
        };
        this.applyRegistrySnapshot(this.registryClient.get());
    }
    
    getSelectedBanks()
    {
        return Object.keys(this.selectedBanks).map((key) => {
            return this.selectedBanks[key];
        }, this);
    }
    
    clearBankSelection()
    {
        this.selectedBanks = {};
        this.applyRegistrySnapshot(this.registryClient.get());
    }
    
    toggleLinkEditing()
    {
        this.linkEditing = !this.linkEditing;
        if (!this.linkEditing) this.selectedBanks = {};
        this.refreshActions();
        this.notify();
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
        if (state.linkEditing !== undefined) this.linkEditing = Boolean(state.linkEditing);
        if (state.editAction !== undefined) this.editAction = state.editAction;
        if (state.clearAction !== undefined) this.clearAction = state.clearAction;
        this.notify();
    }
    
    destroy()
    {
        if (this.unsubscribeRegistry) this.unsubscribeRegistry();
        this.listeners = [];
    }
}


module.exports = {
    BankManagerViewModel: BankManagerViewModel
};
