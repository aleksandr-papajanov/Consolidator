include("Project:/js/Bindings/ControlBinding.js");

function BankManagerControlBinding(controller, presenter, sendMessage) {
    ControlBinding.call(this, presenter, sendMessage);
    this.controller = controller;
    this.hasPresentation = false;
    this.connectPresentation();
}

BankManagerControlBinding.prototype = Object.create(ControlBinding.prototype);
BankManagerControlBinding.prototype.constructor = BankManagerControlBinding;

BankManagerControlBinding.prototype.colorArguments = function (color) {
    if (!color || color.length < 3) {
        return [0, 0, 0, 0, 0];
    }
    return [1, color[0], color[1], color[2],
        color[3] === undefined ? 1 : color[3]];
};

BankManagerControlBinding.prototype.applyPresentation = function (presentation) {
    if (this.hasPresentation && presentation.delta) {
        this.applyDelta(presentation, presentation.delta);
        return;
    }

    var self = this;
    this.send("presentation_begin", [
        presentation.enabled ? 1 : 0,
        presentation.linkEditing ? 1 : 0
    ]);
    (presentation.rows || []).forEach(function (row, rowIndex) {
        self.send("row", [
            rowIndex,
            row.instanceId,
            row.label || "",
            row.local ? 1 : 0
        ]);
        (row.banks || []).forEach(function (bank) {
            self.send("bank", [
                rowIndex,
                bank.bankId,
                bank.label || "",
                bank.system ? 1 : 0,
                bank.visible ? 1 : 0,
                bank.enabled ? 1 : 0,
                bank.active ? 1 : 0,
                bank.opacity === undefined ? 1 : bank.opacity
            ].concat(
                self.colorArguments(bank.color),
                self.colorArguments(bank.textColor)
            ));
        });
    });
    (presentation.linkGroups || []).forEach(function (group) {
        self.send("link_group", [
            group.linkId,
            group.label || "",
            group.active ? 1 : 0,
            group.used ? 1 : 0,
            group.enabled ? 1 : 0
        ].concat(self.colorArguments(group.color)));
    });
    var edit = presentation.editAction || {};
    var clear = presentation.clearAction || {};
    this.send("edit_action", [edit.enabled ? 1 : 0, edit.active ? 1 : 0]);
    this.send("clear_action", [clear.enabled ? 1 : 0, clear.armed ? 1 : 0]);
    this.send("presentation_end");
    this.hasPresentation = true;
};

BankManagerControlBinding.prototype.applyDelta = function (
    presentation,
    delta
) {
    var rowIndex = Number(delta.rowIndex);
    this.send("presentation_patch_begin", [
        presentation.enabled ? 1 : 0,
        presentation.linkEditing ? 1 : 0
    ]);

    if (delta.selector === "bank_focus_changed") {
        this.sendFocusedBankPatch(
            presentation,
            delta.previousRowIndex,
            delta.previousBankId);
        this.sendFocusedBankPatch(
            presentation,
            delta.rowIndex,
            delta.bankId);
        this.send("presentation_patch_end");
        return;
    }

    if (delta.selector === "registry_instance_removed") {
        if (isFinite(rowIndex) && rowIndex >= 0) {
            this.send("row_remove", [rowIndex]);
        }
    } else {
        var row = (presentation.rows || [])[rowIndex];
        if (row) {
            this.sendRow("row_patch", row, rowIndex);
            if (delta.selector === "registry_instance_added") {
                this.sendBanks("bank_patch", row, rowIndex);
            } else if (delta.selector === "registry_bank_group_changed") {
                var bankId = Number(delta.args[4]);
                var bank = (row.banks || []).filter(function (candidate) {
                    return Number(candidate.bankId) === bankId;
                })[0];
                if (bank) this.sendBank("bank_patch", bank, rowIndex);
            }
        }
    }

    if (delta.selector !== "registry_label_changed") {
        this.sendLinkGroups("link_group_patch", presentation.linkGroups);
        this.sendActions(presentation);
    }
    this.send("presentation_patch_end");
};

BankManagerControlBinding.prototype.sendFocusedBankPatch = function (
    presentation,
    rowIndex,
    bankId
) {
    var row = (presentation.rows || [])[Number(rowIndex)];
    if (!row) return;
    var targetBankId = Number(bankId);
    var bank = (row.banks || []).filter(function (candidate) {
        return Number(candidate.bankId) === targetBankId;
    })[0];
    if (bank) this.sendBank("bank_patch", bank, Number(rowIndex));
};

BankManagerControlBinding.prototype.sendRow = function (
    selector,
    row,
    rowIndex
) {
    this.send(selector, [
        rowIndex,
        row.instanceId,
        row.label || "",
        row.local ? 1 : 0
    ]);
};

BankManagerControlBinding.prototype.sendBank = function (
    selector,
    bank,
    rowIndex
) {
    this.send(selector, [
        rowIndex,
        bank.bankId,
        bank.label || "",
        bank.system ? 1 : 0,
        bank.visible ? 1 : 0,
        bank.enabled ? 1 : 0,
        bank.active ? 1 : 0,
        bank.opacity === undefined ? 1 : bank.opacity
    ].concat(
        this.colorArguments(bank.color),
        this.colorArguments(bank.textColor)
    ));
};

BankManagerControlBinding.prototype.sendBanks = function (
    selector,
    row,
    rowIndex
) {
    var self = this;
    (row.banks || []).forEach(function (bank) {
        self.sendBank(selector, bank, rowIndex);
    });
};

BankManagerControlBinding.prototype.sendLinkGroups = function (
    selector,
    groups
) {
    var self = this;
    (groups || []).forEach(function (group) {
        self.send(selector, [
            group.linkId,
            group.label || "",
            group.active ? 1 : 0,
            group.used ? 1 : 0,
            group.enabled ? 1 : 0
        ].concat(self.colorArguments(group.color)));
    });
};

BankManagerControlBinding.prototype.sendActions = function (presentation) {
    var edit = presentation.editAction || {};
    var clear = presentation.clearAction || {};
    this.send("edit_action_patch", [
        edit.enabled ? 1 : 0,
        edit.active ? 1 : 0
    ]);
    this.send("clear_action_patch", [
        clear.enabled ? 1 : 0,
        clear.armed ? 1 : 0
    ]);
};

BankManagerControlBinding.prototype.handleIntent = function (name, values) {
    this.controller.handleIntent(name, values);
};
