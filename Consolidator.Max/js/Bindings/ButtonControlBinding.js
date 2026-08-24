include("Project:/js/Bindings/ControlBinding.js");

function ButtonControlBinding(presenter, sendMessage) {
    ControlBinding.call(this, presenter, sendMessage);
    this.connectPresentation();
}

ButtonControlBinding.prototype = Object.create(ControlBinding.prototype);
ButtonControlBinding.prototype.constructor = ButtonControlBinding;

ButtonControlBinding.prototype.applyPresentation = function (presentation) {
    this.send("set", [presentation.value ? 1 : 0]);
    this.send("enabled", [presentation.enabled ? 1 : 0]);
    this.send("active", [presentation.active ? 1 : 0]);
    this.send("mode", [presentation.mode || "toggle"]);
    this.send("label", [presentation.label || ""]);
};

ButtonControlBinding.prototype.handleIntent = function (name, values) {
    if (name === "valueChanged") {
        this.presenter.setValue(Number(values[0]) !== 0);
    }
};
