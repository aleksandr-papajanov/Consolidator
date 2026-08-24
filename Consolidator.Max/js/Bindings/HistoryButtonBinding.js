include("Project:/js/Bindings/ControlBinding.js");

function HistoryButtonBinding(presenter, sendMessage, slot, activate) {
    ControlBinding.call(this, presenter, sendMessage);
    this.slot = slot;
    this.activate = activate;
    this.presentation = null;
    this.connectPresentation();
}

HistoryButtonBinding.prototype = Object.create(ControlBinding.prototype);
HistoryButtonBinding.prototype.constructor = HistoryButtonBinding;

HistoryButtonBinding.prototype.applyPresentation = function (presentation) {
    this.presentation = presentation[this.slot];
    this.send("enabled", [this.presentation.enabled ? 1 : 0]);
    this.send("label", [this.presentation.label]);
    this.send("mode", ["momentary"]);
    this.send("set", [0]);
};

HistoryButtonBinding.prototype.handleIntent = function (name, values) {
    if (name === "valueChanged" && Number(values[0]) !== 0 &&
            this.presentation && this.presentation.enabled) {
        this.activate();
    }
};
