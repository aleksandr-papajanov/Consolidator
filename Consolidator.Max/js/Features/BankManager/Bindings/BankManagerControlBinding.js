const { BankManagerPresentationEncoder } = require("./BankManagerPresentationEncoder.js");
const { ControlBinding } = require("../../../Shared/Bindings/ControlBinding.js");

class BankManagerControlBinding extends ControlBinding
{
    constructor(controller, presenter, sendMessage)
    {
        super(presenter, sendMessage);
        this.controller = controller;
        this.encoder = new BankManagerPresentationEncoder((selector, args) => {
            this.send(selector, args);
        });
        this.connectPresentation();
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
        this.controller.handleIntent(name, values);
    }

    destroy()
    {
        this.controller = null;
        this.encoder = null;
        super.destroy();
    }
}

module.exports = {
    BankManagerControlBinding: BankManagerControlBinding
};
