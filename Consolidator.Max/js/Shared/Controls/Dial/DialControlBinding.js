const { ControlBinding } = require("../../Bindings/ControlBinding.js");
const { DialGestureSession } = require("./DialGestureSession.js");

function sameArray(first, second)
{
    if (first === second) return true;
    if (!first || !second || first.length !== second.length) return false;
    for (let index = 0; index < first.length; index += 1) {
        if (first[index] !== second[index]) return false;
    }
    return true;
}

function sameVisualization(first, second)
{
    if (first === second) return true;
    if (!first || !second || first.type !== second.type) return false;
    return first.type === "level"
        ? first.peak === second.peak && first.smoothed === second.smoothed
        : first.value === second.value;
}

class DialControlBinding extends ControlBinding
{
    constructor(presenter, sendMessage, transactions)
    {
        super(presenter, sendMessage);
        this.gesture = new DialGestureSession(
            presenter, transactions, () => this.send("transactionRejected"));
        this.connectPresentation();
    }

    applyPresentation(presentation)
    {
        if (this.hasPresentation && !this.requiresFullPresentation(presentation)) {
            this.sendScope(presentation);
            this.send("presentation_begin");
            (presentation.rings || []).forEach((ring, index) => {
                if (this.presentation.rings[index].value !== ring.value) {
                    this.send("set", [index, ring.value]);
                }
            });
            this.send("presentation_end");
            this.presentation = presentation;
            return;
        }
        this.send("presentation_begin");
        this.send("enabled", [presentation.enabled ? 1 : 0]);
        this.send("active", [presentation.active ? 1 : 0]);
        this.sendScope(presentation);
        (presentation.rings || []).forEach((ring, index) => {
            this.send("limits", [index, ring.minimum, ring.maximum]);
            this.send("set", [index, ring.value]);
        });
        this.send("presentation_end");
        this.presentation = presentation;
        this.hasPresentation = true;
    }

    sendScope(presentation)
    {
        let color = presentation.scopeColor;
        let hasColor = color && color.length >= 4;
        this.send("scope", [presentation.scopeActive ? 1 : 0, hasColor ? 1 : 0]
            .concat(hasColor ? color : [0, 0, 0, 0]));
    }

    requiresFullPresentation(presentation)
    {
        let previous = this.presentation || {};
        if (Boolean(previous.enabled) !== Boolean(presentation.enabled) ||
                Boolean(previous.active) !== Boolean(presentation.active)) return true;
        let previousRings = previous.rings || [];
        let rings = presentation.rings || [];
        if (previousRings.length !== rings.length) return true;
        for (let index = 0; index < rings.length; index += 1) {
            let first = previousRings[index];
            let second = rings[index];
            if (first.minimum !== second.minimum ||
                    first.maximum !== second.maximum ||
                    !sameVisualization(first.visualization, second.visualization) ||
                    !sameArray(first.color, second.color) ||
                    (first.display || {}).value !== (second.display || {}).value) {
                return true;
            }
        }
        return false;
    }

    handleIntent(name, values)
    {
        switch (name) {
        case "valueChanged": this.gesture.setValue(values[0], values[1]); break;
        case "reset": this.presenter.resetValue(values[0]); break;
        case "gestureBegan": this.gesture.begin(values[0]); break;
        case "gestureEnded": this.gesture.end(values[0]); break;
        }
    }

    destroy()
    {
        this.gesture.destroy();
        this.gesture = null;
        super.destroy();
    }
}

module.exports = {
    DialControlBinding: DialControlBinding
};
