include("ControlBinding.js");

function DialControlBinding(presenter, sendMessage) {
    ControlBinding.call(this, presenter, sendMessage);
    this.connectPresentation();
}

DialControlBinding.prototype = Object.create(ControlBinding.prototype);
DialControlBinding.prototype.constructor = DialControlBinding;

DialControlBinding.prototype.applyPresentation = function (presentation) {
    this.send("enabled", [presentation.enabled ? 1 : 0]);
    this.send("active", [presentation.active ? 1 : 0]);
    this.send("activeIndex", [presentation.activeIndex || 0]);
    this.send("displayIndex", [presentation.displayIndex || 0]);
    this.send("ringCount", [(presentation.rings || []).length]);
    (presentation.rings || []).forEach(function (ring, index) {
        this.send("limits", [index, ring.minimum, ring.maximum]);
        this.send("set", [index, ring.value]);
    }, this);
};

DialControlBinding.prototype.handleIntent = function (name, values) {
    switch (name) {
    case "valueChanged":
        this.presenter.setValue(values[0], values[1]);
        break;
    case "reset":
        this.presenter.resetValue(values[0]);
        break;
    case "gestureBegan":
        this.presenter.beginGesture(values[0]);
        break;
    case "gestureEnded":
        this.presenter.endGesture(values[0]);
        break;
    }
};
