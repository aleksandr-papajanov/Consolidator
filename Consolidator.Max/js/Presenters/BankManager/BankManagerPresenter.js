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
        presentation.rows = (this.read(viewModel.rows, []) || []).map((row) => {
            return {
                instanceId: row.instanceId,
                label: row.label,
                local: Boolean(row.local),
                mute: Boolean(row.mute),
                solo: Boolean(row.solo),
                banks: (row.banks || []).map((bank) => {
                    return {
                        bankId: bank.bankId,
                        label: bank.label,
                        system: Boolean(bank.system),
                        visible: bank.visible === undefined ? true : Boolean(bank.visible),
                        enabled: bank.enabled === undefined
                            ? true : Boolean(bank.enabled),
                        active: Boolean(bank.active),
                        selected: Boolean(bank.selected),
                        focused: Boolean(bank.focused),
                        groupId: bank.groupId === undefined || bank.groupId === null
                            ? null : bank.groupId,
                        effectActive: Boolean(bank.effectActive),
                        color: bank.color || null,
                        textColor: bank.textColor || null,
                        opacity: bank.opacity === undefined ? 1 : bank.opacity
                    };
                })
            };
        });
        presentation.groupAction = this.read(viewModel.groupAction, {
            enabled: false,
            active: false
        });
        presentation.ungroupAction = this.read(viewModel.ungroupAction, {
            enabled: false,
            active: false
        });
        presentation.clearAction = this.read(viewModel.clearAction, {
            enabled: false,
            armed: false
        });
        let history = this.read(viewModel.history, {});
        presentation.history = {
            cursor: Number(history.cursor) || 0,
            entryCount: Number(history.entryCount) || 0,
            canUndo: Boolean(history.canUndo),
            canRedo: Boolean(history.canRedo)
        };
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
