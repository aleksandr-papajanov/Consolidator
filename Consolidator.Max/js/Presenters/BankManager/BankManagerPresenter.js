const { PresentationObservable } = require("../Core/PresentationObservable.js");
const { presentationBindingValue } = require("../Core/PresentationBinding.js");
const { BankManagerPresentation } = require("./BankManagerPresentation.js");

class BankManagerPresenter extends PresentationObservable
{
    constructor(viewModel)
    {
        super();
        this.viewModel = viewModel;
        this.unsubscribers = [];
        this.subscribeViewModel();
        this.rebuild();
    }
    
    subscribeViewModel()
    {
        if (this.viewModel && typeof this.viewModel.subscribe === "function") {
            this.unsubscribers.push(this.viewModel.subscribe((_, delta) => {
                this.rebuild(delta);
            }, false));
        }
    }
    
    read(value, fallback)
    {
        return presentationBindingValue(value, fallback);
    }
    
    rebuild(delta)
    {
        let viewModel = this.viewModel || {};
        let presentation = new BankManagerPresentation();
        presentation.enabled = Boolean(this.read(viewModel.enabled, true));
        let linkEditing = Boolean(this.read(viewModel.linkEditing, false));
        presentation.rows = (this.read(viewModel.rows, []) || []).map((row) => {
            return {
                instanceId: row.instanceId,
                label: row.label,
                local: Boolean(row.local),
                banks: (row.banks || []).map((bank) => {
                    return {
                        bankId: bank.bankId,
                        label: bank.label,
                        system: Boolean(bank.system),
                        visible: bank.visible === undefined ? true : Boolean(bank.visible),
                        enabled: bank.enabled === undefined
                            ? true : Boolean(bank.enabled),
                        active: Boolean(linkEditing
                            ? bank.linkSelected
                            : bank.active),
                        focused: Boolean(!linkEditing && bank.focused),
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
        presentation.linkGroups = (this.read(viewModel.linkGroups, []) || []).map((group) => {
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
        presentation.delta = delta || null;
        this.publish(presentation);
    }
    
    destroy()
    {
        if (this.destroyed) return;
        for (let index = 0; index < this.unsubscribers.length; index += 1) {
            this.unsubscribers[index]();
        }
        this.unsubscribers = [];
        super.destroy();
    }
}


module.exports = {
    BankManagerPresenter: BankManagerPresenter
};
