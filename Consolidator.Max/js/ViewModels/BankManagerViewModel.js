function BankManagerViewModel(registryClient, localInstanceId) {
    this.registryClient = registryClient;
    this.localInstanceId = localInstanceId;
    this.enabled = true;
    this.rows = [];
    this.linkEditing = false;
    this.linkGroups = [];
    this.editAction = { enabled: false, active: false };
    this.clearAction = { enabled: false, armed: false };
    this.listeners = [];
    this.unsubscribeRegistry = registryClient.subscribe(
        this.applyRegistrySnapshot.bind(this), true);
}

BankManagerViewModel.prototype.applyRegistrySnapshot = function (snapshot) {
    if (!snapshot) return;
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
                    linkSelected: false,
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
    this.notify();
};

BankManagerViewModel.prototype.setLocalInstanceId = function (instanceId) {
    if (String(this.localInstanceId) === String(instanceId)) return;
    this.localInstanceId = instanceId;
    this.applyRegistrySnapshot(this.registryClient.get());
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
