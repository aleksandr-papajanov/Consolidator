function BankManagerViewModel(registryClient, localInstanceId) {
    this.registryClient = registryClient;
    this.localInstanceId = localInstanceId;
    this.enabled = true;
    this.rows = [];
    this.linkEditing = false;
    this.selectedBanks = {};
    this.linkGroups = [];
    this.editAction = { enabled: false, active: false };
    this.clearAction = { enabled: false, armed: false };
    this.listeners = [];
    this.unsubscribeRegistry = registryClient.subscribe(
        this.applyRegistrySnapshot.bind(this), true);
}

BankManagerViewModel.prototype.applyRegistrySnapshot = function (snapshot) {
    if (!snapshot) return;
    var selectedBanks = this.selectedBanks;
    this.rows = snapshot.instances.map(function (instance) {
        return {
            instanceId: instance.instanceId,
            label: instance.label,
            local: String(instance.instanceId) === String(this.localInstanceId),
            banks: instance.banks.map(function (bank) {
                var bankId = Number(bank.bankId);
                return {
                    bankId: bankId,
                    label: String(bankId),
                    system: bankId === 1,
                    visible: true,
                    enabled: bankId !== 1,
                    focused: bankId === Number(instance.selectedBank),
                    linkSelected: Boolean(selectedBanks[
                        String(instance.instanceId) + ":" + String(bankId)
                    ]),
                    groupId: bank.groupId,
                    color: null,
                    opacity: 1
                };
            })
        };
    }, this);
    this.linkGroups = snapshot.groups.map(function (group) {
        return {
            linkId: group.groupId,
            label: String(group.groupId),
            active: false,
            used: group.members.length > 0,
            enabled: true,
            members: group.members
        };
    });
    this.refreshActions();
    this.notify();
};

BankManagerViewModel.prototype.refreshActions = function () {
    var localRow = this.rows.filter(function (row) { return row.local; })[0];
    var editableBanks = localRow ? localRow.banks.filter(function (bank) {
        return !bank.system;
    }) : [];
    var editEnabled = editableBanks.length > 0;
    var clearEnabled = editableBanks.some(function (bank) {
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
};

BankManagerViewModel.prototype.setLocalInstanceId = function (instanceId) {
    if (String(this.localInstanceId) === String(instanceId)) return;
    this.localInstanceId = instanceId;
    this.applyRegistrySnapshot(this.registryClient.get());
};

BankManagerViewModel.prototype.toggleBankSelection = function (
    instanceId,
    bankId
) {
    var key = String(instanceId) + ":" + String(bankId);
    if (this.selectedBanks[key]) delete this.selectedBanks[key];
    else this.selectedBanks[key] = {
        instanceId: instanceId,
        bankId: Number(bankId)
    };
    this.applyRegistrySnapshot(this.registryClient.get());
};

BankManagerViewModel.prototype.getSelectedBanks = function () {
    return Object.keys(this.selectedBanks).map(function (key) {
        return this.selectedBanks[key];
    }, this);
};

BankManagerViewModel.prototype.clearBankSelection = function () {
    this.selectedBanks = {};
    this.applyRegistrySnapshot(this.registryClient.get());
};

BankManagerViewModel.prototype.toggleLinkEditing = function () {
    this.linkEditing = !this.linkEditing;
    if (!this.linkEditing) this.selectedBanks = {};
    this.refreshActions();
    this.notify();
};

BankManagerViewModel.prototype.subscribe = function (callback, immediate) {
    this.listeners.push(callback);
    if (immediate) callback(this);
    var self = this;
    return function () {
        self.listeners = self.listeners.filter(function (listener) {
            return listener !== callback;
        });
    };
};

BankManagerViewModel.prototype.notify = function () {
    var listeners = this.listeners.slice();
    for (var index = 0; index < listeners.length; index += 1) {
        listeners[index](this);
    }
};

BankManagerViewModel.prototype.apply = function (state) {
    state = state || {};
    if (state.enabled !== undefined) this.enabled = Boolean(state.enabled);
    if (state.linkEditing !== undefined) this.linkEditing = Boolean(state.linkEditing);
    if (state.editAction !== undefined) this.editAction = state.editAction;
    if (state.clearAction !== undefined) this.clearAction = state.clearAction;
    this.notify();
};

BankManagerViewModel.prototype.destroy = function () {
    if (this.unsubscribeRegistry) this.unsubscribeRegistry();
    this.listeners = [];
};
