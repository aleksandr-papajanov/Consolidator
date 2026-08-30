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
        let color = presentation.scopeColor;
        this.send("scope", [presentation.scopeActive ? 1 : 0,
            color && color.length >= 4 ? 1 : 0].concat(
            color && color.length >= 4 ? color : [0, 0, 0, 0]));
        this.send("mode", [presentation.mode || "toggle"]);
        this.send("label", [presentation.label || ""]);
        this.send("presentation_end");
    }
    
    handleIntent(name, values)
    {
        if (name === "valueChanged") {
            this.presenter.setValue(Number(values[0]) !== 0);
        }
        else if (name === "reset") {
            this.presenter.resetValue();
        }
    }
}

module.exports = {
    ButtonControlBinding: ButtonControlBinding
};
