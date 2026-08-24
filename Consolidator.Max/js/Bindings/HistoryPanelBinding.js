include("Project:/js/Bindings/ControlBinding.js");

function HistoryPanelBinding(presenter, sendMessage, select) {
    ControlBinding.call(this, presenter, sendMessage);
    this.select = select;
    this.connectPresentation();
}

HistoryPanelBinding.prototype = Object.create(ControlBinding.prototype);
HistoryPanelBinding.prototype.constructor = HistoryPanelBinding;

HistoryPanelBinding.prototype.applyPresentation = function (presentation) {
    var self = this;
    this.send("presentation_begin", [presentation.cursor || 0]);
    (presentation.entries || []).forEach(function (entry, index) {
        self.send("entry", [
            index,
            entry.kind,
            entry.label,
            index < (presentation.cursor || 0) ? 1 : 0,
            index === (presentation.cursor || 0) - 1 ? 1 : 0
        ]);
    });
    this.send("presentation_end");
};

HistoryPanelBinding.prototype.handleIntent = function (name, values) {
    if (name === "historySelected") {
        if (this.select) this.select(Number(values[0]));
    }
};
