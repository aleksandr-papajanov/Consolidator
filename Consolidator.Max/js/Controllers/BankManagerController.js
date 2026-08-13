include("BankManagerContext.js");

function BankManagerController(context) {
    this.context = context;
    this.clearConfirmationArmed = false;
    this.clearConfirmationTimer = null;
}

BankManagerController.prototype.updateClearAction = function (armed) {
    this.clearConfirmationArmed = armed;
    this.context.viewModel.apply({
        clearAction: {
            enabled: Boolean(this.context.viewModel.clearAction.enabled),
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
    if (this.clearConfirmationTimer !== null &&
            typeof clearTimeout === "function") {
        clearTimeout(this.clearConfirmationTimer);
    }
    if (typeof setTimeout === "function") {
        this.clearConfirmationTimer = setTimeout(function () {
            self.disarmClearConfirmation();
        }, 3000);
    }
};

BankManagerController.prototype.isLocal = function (instanceId) {
    return String(instanceId) === String(this.context.instanceId);
};

BankManagerController.prototype.selectBank = function (instanceId, bankId) {
    if (this.context.viewModel.linkEditing) {
        this.toggleBankSelection(instanceId, bankId);
        return;
    }
    this.context.analyzer.show(instanceId, bankId);
    if (this.isLocal(instanceId)) this.context.selectedBank.set(bankId);
};

BankManagerController.prototype.selectRow = function (instanceId) {
    var row = this.context.viewModel.rows.filter(function (candidate) {
        return String(candidate.instanceId) === String(instanceId);
    })[0];
    if (!row) return;
    var focused = row.banks.filter(function (bank) { return bank.focused; })[0];
    if (focused) this.selectBank(instanceId, focused.bankId);
};

BankManagerController.prototype.toggleBankSelection = function (
    instanceId,
    bankId
) {
    this.context.viewModel.toggleBankSelection(instanceId, bankId);
};

BankManagerController.prototype.toggleLinkEditing = function () {
    this.context.viewModel.toggleLinkEditing();
};

BankManagerController.prototype.applyLinkGroup = function (linkId) {
    var context = this.context;
    var entries = context.viewModel.getSelectedBanks()
        .filter(function (selection) {
            return String(selection.instanceId) === String(context.instanceId) &&
                Number(selection.bankId) !== 1;
        })
        .map(function (selection) {
            return {
                path: "bank." + selection.bankId + ".group",
                value: Number(linkId)
            };
        });
    if (entries.length) context.state.setMany(entries);
    context.viewModel.clearBankSelection();
    context.viewModel.toggleLinkEditing();
};

BankManagerController.prototype.clearAll = function () {
    if (!this.clearConfirmationArmed) {
        this.armClearConfirmation();
        return;
    }
    this.disarmClearConfirmation();
    var entries = [];
    for (var bankId = 2; bankId <= 7; bankId += 1) {
        entries.push({ path: "bank." + bankId + ".group", value: null });
    }
    this.context.state.setMany(entries);
};

BankManagerController.prototype.handleIntent = function (name, values) {
    values = values || [];
    switch (name) {
    case "bankSelected": this.selectBank(values[0], values[1]); break;
    case "rowSelected": this.selectRow(values[0]); break;
    case "linkGroupSelected": this.applyLinkGroup(values[0]); break;
    case "editToggled": this.toggleLinkEditing(); break;
    case "clearRequested": this.clearAll(); break;
    }
};

BankManagerController.prototype.destroy = function () {
    if (this.clearConfirmationTimer !== null &&
            typeof clearTimeout === "function") {
        clearTimeout(this.clearConfirmationTimer);
    }
    this.clearConfirmationTimer = null;
    this.context = null;
};
