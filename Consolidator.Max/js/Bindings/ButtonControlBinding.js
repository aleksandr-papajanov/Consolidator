const { ControlBinding } = require("./ControlBinding.js");

class ButtonControlBinding extends ControlBinding
{
    constructor(presenter, sendMessage)
    {
        super(presenter, sendMessage);
        this.connectPresentation();
    }
    
    applyPresentation(presentation)
    {
        this.send("presentation_begin");
        this.send("set", [presentation.value ? 1 : 0]);
        this.send("enabled", [presentation.enabled ? 1 : 0]);
        this.send("active", [presentation.active ? 1 : 0]);
        this.send("mode", [presentation.mode || "toggle"]);
        this.send("label", [presentation.label || ""]);
        this.send("presentation_end");
    }
    
    handleIntent(name, values)
    {
        if (name === "valueChanged") {
            this.presenter.setValue(Number(values[0]) !== 0);
        }
    }
}

module.exports = {
    ButtonControlBinding: ButtonControlBinding
};
