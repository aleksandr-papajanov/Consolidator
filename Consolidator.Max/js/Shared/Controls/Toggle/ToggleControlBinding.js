const { ControlBinding } = require("../../Bindings/ControlBinding.js");

class ToggleControlBinding extends ControlBinding
{
    constructor(presenter, sendMessage, transactions)
    {
        super(presenter, sendMessage);
        this.transactions = transactions;
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
        this.send("label", [presentation.label || ""]);
        this.send("presentation_end");
    }

    handleIntent(name, values)
    {
        if (name === "valueChanged") {
            let value = Number(values[0]) !== 0;
            if (!this.transactions) {
                this.presenter.setValue(value);
                return;
            }
            this.transactions.begin((transactionId, response) => {
                if (response && response.status === "accepted") {
                    this.presenter.setValue(value, transactionId);
                    this.transactions.end(transactionId);
                }
            });
        }
        else if (name === "reset") {
            this.presenter.resetValue();
        }
    }

    destroy()
    {
        this.transactions = null;
        super.destroy();
    }
}

module.exports = { ToggleControlBinding: ToggleControlBinding };
