const { BankManagerPresentationEncoder } = require("./BankManagerPresentationEncoder.js");
const { ControlBinding } = require("../../../Shared/Bindings/ControlBinding.js");
const { uiEditPrecision } = require(
    "../../../Shared/ViewModels/UiEditPrecision.js");

class BankManagerControlBinding extends ControlBinding
{
    constructor(controller, presenter, sendMessage, precisionMode)
    {
        super(presenter, sendMessage);
        this.controller = controller;
        this.encoder = new BankManagerPresentationEncoder((selector, args) => {
            this.send(selector, args);
        });
        this.connectPresentation();
        this.precisionMode = precisionMode || uiEditPrecision;
        this.precisionUnsubscriber = this.precisionMode.subscribe((presentation) => {
            this.send("edit_precision", [presentation.value ? 1 : 0]);
        }, true);
    }

    applyPresentation(presentation)
    {
        if (this.hasPresentation && presentation.delta)
        {
            this.encoder.sendDelta(presentation, presentation.delta);
        }
        else
        {
            this.encoder.sendPresentation(presentation);
        }
        this.hasPresentation = true;
    }

    refreshPresentation()
    {
        const presentation = this.pendingPresentation ||
            (this.presenter && this.presenter.presentation);
        this.pendingPresentation = null;
        if (!presentation)
        {
            return;
        }
        this.hasPresentation = false;
        this.applyPresentation(presentation);
    }

    handleIntent(name, values)
    {
        if (name === "editPrecisionToggled")
        {
            this.precisionMode.toggle();
            return;
        }
        this.controller.handleIntent(name, values);
    }

    destroy()
    {
        this.controller = null;
        this.encoder = null;
        this.precisionMode = null;
        if (this.precisionUnsubscriber) this.precisionUnsubscriber();
        this.precisionUnsubscriber = null;
        super.destroy();
    }
}

module.exports = {
    BankManagerControlBinding: BankManagerControlBinding
};
