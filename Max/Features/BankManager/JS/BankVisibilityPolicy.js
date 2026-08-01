function BankVisibilityPolicy(manager) {
    this.manager = manager;
}

BankVisibilityPolicy.prototype.Rows = function() {
    return this.manager.Rows();
};

BankVisibilityPolicy.prototype.IsVisible = function(instance, bank, local) {
    if (local || this.manager.linkEditingEnabled) return true;
    return this.manager.IsActiveGroupMember(bank);
};

BankVisibilityPolicy.prototype.Opacity = function(bank, local, selected, editSelected) {
    var manager = this.manager;
    if (manager.linkEditingEnabled) {
        return bank.id === 0 || bank.id === 1 || bank.id === 6 ? 0.0 : 1.0;
    }
    if (!bank.occupied && !manager.IsActiveGroupMember(bank) && !selected && !editSelected) {
        return BankManagerVisualOptions.inactiveBankOpacity;
    }
    return 1.0;
};

BankVisibilityPolicy.prototype.IsEnabled = function(bank, local) {
    if (!bank || bank.id === 0) return false;
    return this.manager.linkEditingEnabled
        ? this.manager.groupOperations.IsEditableBank(bank)
        : local;
};
