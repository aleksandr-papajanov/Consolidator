const { ControlBinding } = require("./ControlBinding.js");

class MultiValueToggleControlBinding extends ControlBinding
{
    constructor(presenter, sendMessage)
    {
        super(presenter, sendMessage);
        this.connectPresentation();
    }

    applyPresentation(presentation)
    {
        this.send("presentation_begin");
        this.send("set", [presentation.value]);
        this.send("values", presentation.values || []);
        this.send("enabled", [presentation.enabled ? 1 : 0]);
        this.send("active", [presentation.active ? 1 : 0]);
        this.send("presentation_end");
    }

    handleIntent(name, values)
    {
        if (name === "valueChanged") this.presenter.setValue(Number(values[0]));
        else if (name === "reset") this.presenter.resetValue();
    }
}

module.exports = { MultiValueToggleControlBinding: MultiValueToggleControlBinding };
