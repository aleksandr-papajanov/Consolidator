include("Project:/js/Controllers/BankManagerContext.js");

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

BankManagerController.prototype.selectBank = function (
    instanceId,
    bankId,
    extendSelection
) {
    if (this.context.viewModel.linkEditing) {
        this.toggleBankSelection(instanceId, bankId, extendSelection);
        return;
    }
    if (this.context.viewModel.setFocusedBank(instanceId, bankId) === false) {
        return;
    }
    this.context.uiTarget.show(instanceId, bankId);
};

BankManagerController.prototype.selectRow = function (instanceId) {
    var row = this.context.viewModel.rows.filter(function (candidate) {
        return String(candidate.instanceId) === String(instanceId);
    })[0];
    if (!row || !row.banks.length) return;
    this.selectBank(instanceId, row.banks[0].bankId);
};

BankManagerController.prototype.toggleBankSelection = function (
    instanceId,
    bankId,
    extendSelection
) {
    this.context.viewModel.toggleBankSelection(
        instanceId,
        bankId,
        extendSelection
    );
};

BankManagerController.prototype.toggleLinkEditing = function () {
    this.context.viewModel.toggleLinkEditing();
};

BankManagerController.prototype.applyLinkGroup = function (linkId) {
    var context = this.context;
    var entriesByInstance = {};
    context.viewModel.getSelectedBanks()
        .forEach(function (selection) {
            var instanceId = String(selection.instanceId);
            if (!entriesByInstance[instanceId]) {
                entriesByInstance[instanceId] = [];
            }
            entriesByInstance[instanceId].push({
                path: "bank." + selection.bankId + ".group",
                value: Number(linkId)
            });
        });
    Object.keys(entriesByInstance).forEach(function (instanceId) {
        context.state.setManyFor(
            instanceId,
            entriesByInstance[instanceId]
        );
    });
    context.viewModel.clearBankSelection();
    context.viewModel.toggleLinkEditing();
};

BankManagerController.prototype.clearLocalGroups = function () {
    if (!this.clearConfirmationArmed) {
        this.armClearConfirmation();
        return;
    }
    this.disarmClearConfirmation();
    var entries = [];
    for (var bankId = 0; bankId < 7; bankId += 1) {
        entries.push({ path: "bank." + bankId + ".group", value: null });
    }
    this.context.state.setManyFor(this.context.instanceId, entries);
};

BankManagerController.prototype.handleIntent = function (name, values) {
    values = values || [];
    switch (name) {
    case "bankSelected":
        this.selectBank(values[0], values[1], Number(values[2]) !== 0);
        break;
    case "rowSelected": this.selectRow(values[0]); break;
    case "linkGroupSelected": this.applyLinkGroup(values[0]); break;
    case "editToggled": this.toggleLinkEditing(); break;
    case "clearRequested": this.clearLocalGroups(); break;
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
