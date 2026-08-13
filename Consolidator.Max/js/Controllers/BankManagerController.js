function BankManagerController(viewModel, rootViewModel) {
    this.viewModel = viewModel;
    this.rootViewModel = rootViewModel;
    this.clearConfirmationArmed = false;
    this.clearConfirmationTimer = null;
}

BankManagerController.prototype.updateClearAction = function (armed) {
    this.clearConfirmationArmed = armed;
    this.viewModel.apply({
        clearAction: {
            enabled: Boolean(this.viewModel.clearAction.enabled),
            armed: armed
        }
    });
};

BankManagerController.prototype.disarmClearConfirmation = function () {
    this.clearConfirmationTimer = null;
    this.updateClearAction(false);
};

BankManagerController.prototype.armClearConfirmation = function () {
    var self = this;
    this.updateClearAction(true);
    if (this.clearConfirmationTimer !== null
            && typeof clearTimeout === "function") {
        clearTimeout(this.clearConfirmationTimer);
    }
    if (typeof setTimeout === "function") {
        this.clearConfirmationTimer = setTimeout(function () {
            self.disarmClearConfirmation();
        }, 3000);
    }
};

BankManagerController.prototype.selectBank = function (instanceId, bankId) {
    if (this.viewModel.linkEditing) {
        this.toggleBankSelection(instanceId, bankId);
        return;
    }

    this.rootViewModel.analyzer.show(instanceId, bankId);
    if (instanceId === this.rootViewModel.instanceId) {
        this.rootViewModel.selectedBank.set(bankId);
    }
};

BankManagerController.prototype.selectRow = function (instanceId) {
    this.rootViewModel.selectInstance(instanceId);
};

BankManagerController.prototype.toggleBankSelection = function (instanceId, bankId) {
    this.rootViewModel.bankSelection.toggle(instanceId, bankId);
};

BankManagerController.prototype.toggleLinkEditing = function () {
    this.rootViewModel.toggleBankLinkEditing();
};

BankManagerController.prototype.applyLinkGroup = function (linkId) {
    this.rootViewModel.applyBankLinkGroup(linkId);
};

BankManagerController.prototype.clearAll = function () {
    if (!this.clearConfirmationArmed) {
        this.armClearConfirmation();
        return;
    }
    this.disarmClearConfirmation();
    this.rootViewModel.clearAllBanks();
};

BankManagerController.prototype.handleIntent = function (name, values) {
    values = values || [];
    switch (name) {
    case "bankSelected":
        this.selectBank(values[0], values[1]);
        break;
    case "rowSelected":
        this.selectRow(values[0]);
        break;
    case "linkGroupSelected":
        this.applyLinkGroup(values[0]);
        break;
    case "editToggled":
        this.toggleLinkEditing();
        break;
    case "clearRequested":
        this.clearAll();
        break;
    }
};
