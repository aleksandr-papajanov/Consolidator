include("../Core/PresentationObservable.js");
include("../Core/PresentationBinding.js");
include("BankManagerPresentation.js");

function BankManagerPresenter(viewModel) {
    PresentationObservable.call(this);
    this.viewModel = viewModel;
    this.unsubscribers = [];
    this.subscribeViewModel();
    this.rebuild();
}

BankManagerPresenter.prototype = Object.create(PresentationObservable.prototype);
BankManagerPresenter.prototype.constructor = BankManagerPresenter;

BankManagerPresenter.prototype.subscribeViewModel = function () {
    var self = this;
    if (this.viewModel && typeof this.viewModel.subscribe === "function") {
        this.unsubscribers.push(this.viewModel.subscribe(function () {
            self.rebuild();
        }, false));
    }
};

BankManagerPresenter.prototype.read = function (value, fallback) {
    return presentationBindingValue(value, fallback);
};

BankManagerPresenter.prototype.rebuild = function () {
    var viewModel = this.viewModel || {};
    var presentation = new BankManagerPresentation();
    presentation.enabled = Boolean(this.read(viewModel.enabled, true));
    var linkEditing = Boolean(this.read(viewModel.linkEditing, false));
    presentation.rows = (this.read(viewModel.rows, []) || []).map(function (row) {
        return {
            instanceId: row.instanceId,
            label: row.label,
            local: Boolean(row.local),
            banks: (row.banks || []).map(function (bank) {
                return {
                    bankId: bank.bankId,
                    label: bank.label,
                    system: Boolean(bank.system),
                    visible: bank.visible === undefined ? true : Boolean(bank.visible),
                    enabled: bank.enabled === undefined
                        ? bank.bankId !== 1 : Boolean(bank.enabled),
                    active: Boolean(
                        (bank.bankId !== 1 && !linkEditing && bank.focused)
                        || bank.linkSelected
                    ),
                    focused: Boolean(bank.bankId !== 1 && !linkEditing && bank.focused),
                    linkSelected: Boolean(bank.linkSelected),
                    linkId: bank.groupId === undefined || bank.groupId === null
                        ? null : bank.groupId,
                    color: bank.color || null,
                    textColor: bank.textColor || null,
                    opacity: bank.opacity === undefined ? 1 : bank.opacity
                };
            })
        };
    });
    presentation.linkEditing = linkEditing;
    presentation.linkGroups = (this.read(viewModel.linkGroups, []) || []).map(function (group) {
        return {
            linkId: group.linkId,
            label: group.label,
            active: Boolean(linkEditing
                ? group.selectionActive
                : group.activeLink === undefined ? group.active : group.activeLink),
            used: Boolean(group.used),
            enabled: Boolean(group.enabled),
            color: group.color || null
        };
    });
    presentation.editAction = this.read(viewModel.editAction, {
        enabled: false,
        active: presentation.linkEditing
    });
    presentation.clearAction = this.read(viewModel.clearAction, {
        enabled: false,
        armed: false
    });
    this.publish(presentation);
};

BankManagerPresenter.prototype.destroy = function () {
    if (this.destroyed) return;
    for (var index = 0; index < this.unsubscribers.length; index += 1) {
        this.unsubscribers[index]();
    }
    this.unsubscribers = [];
    PresentationObservable.prototype.destroy.call(this);
};
