include("Project:/js/Bindings/ControlBinding.js");

function BankManagerControlBinding(controller, presenter, sendMessage) {
    ControlBinding.call(this, presenter, sendMessage);
    this.controller = controller;
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
};

BankManagerControlBinding.prototype.handleIntent = function (name, values) {
    this.controller.handleIntent(name, values);
};
