function BankManagerSelection(manager) {
    this.manager = manager;
    this.focusedInstanceId = "";
    this.focusedBankId = 1;
}

BankManagerSelection.prototype.ActiveBank = function(instance) {
    return instance && instance.banks[instance.selectedBankId - 1]
        ? instance.banks[instance.selectedBankId - 1]
        : null;
};

BankManagerSelection.prototype.ActiveLinkId = function(instance) {
    var bank = this.ActiveBank(instance);
    return bank ? bank.linkId : "";
};

BankManagerSelection.prototype.FocusedInstance = function() {
    var manager = this.manager;
    if (!this.focusedInstanceId || this.focusedInstanceId === manager.instanceId) {
        return manager.local;
    }
    return manager.peers[this.focusedInstanceId] || manager.local;
};

BankManagerSelection.prototype.FocusedBank = function() {
    var instance = this.FocusedInstance();
    return instance && instance.banks[this.focusedBankId - 1]
        ? instance.banks[this.focusedBankId - 1]
        : null;
};

BankManagerSelection.prototype.SetFocusedBank = function(instance, bankId) {
    if (!instance || !isFinite(bankId) || bankId < 1 || bankId > 6) return false;
    this.focusedInstanceId = instance.id;
    this.focusedBankId = bankId;
    return true;
};

BankManagerSelection.prototype.Rows = function() {
    var manager = this.manager;
    var rows = [manager.local].concat(Object.keys(manager.peers).map(function(id) {
        return manager.peers[id];
    }));
    rows.sort(function(left, right) {
        if (left.trackOrder !== right.trackOrder) return left.trackOrder - right.trackOrder;
        return left.id < right.id ? -1 : (left.id > right.id ? 1 : 0);
    });
    return rows;
};
