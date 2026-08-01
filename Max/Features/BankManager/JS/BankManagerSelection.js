function BankManagerSelection(manager) {
    this.manager = manager;
    this.focusedInstanceId = "";
    this.focusedBankId = 1;
    this.editSelection = {};
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

BankManagerSelection.prototype.EditKey = function(instance, bank) {
    return String(instance.id) + ":" + String(bank.id);
};

BankManagerSelection.prototype.ClearEditSelection = function() {
    this.editSelection = {};
};

BankManagerSelection.prototype.ToggleEditBank = function(instance, bank, extend) {
    if (!extend) this.ClearEditSelection();
    var key = this.EditKey(instance, bank);
    if (this.editSelection[key]) delete this.editSelection[key];
    else this.editSelection[key] = true;
};

BankManagerSelection.prototype.IsEditSelected = function(instance, bank) {
    return Boolean(instance && bank && this.editSelection[this.EditKey(instance, bank)]);
};

BankManagerSelection.prototype.EditSelection = function() {
    var manager = this.manager;
    var result = [];
    var rows = this.Rows();
    for (var rowIndex = 0; rowIndex < rows.length; ++rowIndex) {
        var instance = rows[rowIndex];
        for (var bankIndex = 0; bankIndex < instance.banks.length; ++bankIndex) {
            var bank = instance.banks[bankIndex];
            if (this.IsEditSelected(instance, bank)) {
                result.push({ instance: instance, bank: bank });
            }
        }
    }
    return result;
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
