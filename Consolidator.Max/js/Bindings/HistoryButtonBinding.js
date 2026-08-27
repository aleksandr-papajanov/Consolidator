const { ControlBinding } = require("./ControlBinding.js");

class HistoryButtonBinding extends ControlBinding
{
    constructor(presenter, sendMessage, slot, activate)
    {
        super(presenter, sendMessage);
        this.slot = slot;
        this.activate = activate;
        this.presentation = null;
        this.connectPresentation();
    }
    
    applyPresentation(presentation)
    {
        this.presentation = presentation[this.slot];
        this.send("enabled", [this.presentation.enabled ? 1 : 0]);
        this.send("label", [this.presentation.label]);
        this.send("mode", ["momentary"]);
        this.send("set", [0]);
    }
    
    handleIntent(name, values)
    {
        if (name === "valueChanged" && Number(values[0]) !== 0 &&
                this.presentation && this.presentation.enabled) {
            this.activate();
        }
    }
}

module.exports = {
    HistoryButtonBinding: HistoryButtonBinding
};
