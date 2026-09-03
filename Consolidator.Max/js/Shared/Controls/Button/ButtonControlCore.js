const { ButtonPresentation } = require(
    "./ButtonPresentation.js");
const { DoubleClickTracker } = require("../DoubleClickTracker.js");
const { ButtonRenderer } = require("./ButtonRenderer.js");

class ButtonControlCore
{
    constructor()
    {
        this.presentation = new ButtonPresentation();
        this.pressed = false;
        this.presentationDepth = 0;
        this.presentationDirty = false;
        this.doubleClick = new DoubleClickTracker();
        this.renderer = new ButtonRenderer();
    }

    requestRedraw()
    {
        if (this.presentationDepth > 0) {
            this.presentationDirty = true;
            return;
        }
        mgraphics.redraw();
    }

    beginPresentation() { this.presentationDepth += 1; }

    endPresentation()
    {
        if (this.presentationDepth === 0) return;
        this.presentationDepth -= 1;
        if (this.presentationDepth === 0 && this.presentationDirty) {
            this.presentationDirty = false;
            mgraphics.redraw();
        }
    }

    applyPresentation(presentation)
    {
        if (!presentation) return;
        this.presentation = presentation;
        if (!presentation.enabled || presentation.mode !== "momentary") {
            this.pressed = false;
        }
        this.requestRedraw();
    }

    setPresentationValue(value) {
        this.presentation.value = Number(value) !== 0;
        this.requestRedraw();
    }
    setPresentationEnabled(value) {
        this.presentation.enabled = Number(value) !== 0;
        this.requestRedraw();
    }
    setPresentationActive(value) {
        this.presentation.active = Number(value) !== 0;
        this.requestRedraw();
    }
    setPresentationMode(value) {
        this.presentation.mode = String(value) === "momentary" ? "momentary" : "toggle";
        this.requestRedraw();
    }
    setPresentationLabel(value) {
        this.presentation.label = String(value);
        this.requestRedraw();
    }
    setScope(active, hasColor, red, green, blue, alpha) {
        this.presentation.scopeActive = Number(active) !== 0;
        this.presentation.scopeColor = Number(hasColor) !== 0
            ? [Number(red), Number(green), Number(blue), Number(alpha)] : null;
        this.requestRedraw();
    }
    emit(name, payload) {
        if (payload === undefined) outlet(0, name);
        else if (payload instanceof Array) outlet(0, [name].concat(payload));
        else outlet(0, [name, payload]);
    }
    paint() { this.renderer.paint(this.presentation); }

    click()
    {
        if (!this.presentation.enabled) return;
        if (this.doubleClick.isDoubleClick("button")) {
            this.emit("reset");
            return;
        }
        if (this.presentation.mode === "momentary") {
            this.pressed = true;
            this.emit("valueChanged", 1);
            return;
        }
        this.emit("valueChanged", this.presentation.value ? 0 : 1);
    }

    release()
    {
        if (!this.pressed) return;
        this.pressed = false;
        this.emit("valueChanged", 0);
    }
}

module.exports = {
    ButtonControlCore: ButtonControlCore
};
