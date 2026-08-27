const { ControlBinding } = require("./ControlBinding.js");

class HistoryPanelBinding extends ControlBinding
{
    constructor(presenter, sendMessage, select)
    {
        super(presenter, sendMessage);
        this.select = select;
        this.connectPresentation();
    }
    
    applyPresentation(presentation)
    {
        this.send("presentation_begin", [presentation.cursor || 0]);
        (presentation.entries || []).forEach((entry, index) => {
            this.send("entry", [
                index,
                entry.kind,
                entry.label,
                index < (presentation.cursor || 0) ? 1 : 0,
                index === (presentation.cursor || 0) - 1 ? 1 : 0
            ]);
        });
        this.send("presentation_end");
    }
    
    handleIntent(name, values)
    {
        if (name === "historySelected") {
            if (this.select) this.select(Number(values[0]));
        }
    }
}

module.exports = {
    HistoryPanelBinding: HistoryPanelBinding
};
