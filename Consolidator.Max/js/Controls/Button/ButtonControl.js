autowatch = 1;
inlets = 1;
outlets = 1;

mgraphics.init();
mgraphics.relative_coords = 0;
mgraphics.autofill = 0;

const { ButtonPresentation } = require("../../Presenters/Button/ButtonPresentation.js");
const { DoubleClickTracker } = require("../DoubleClickTracker.js");
const { UiColors } = require("../../Theme/UiColors.js");

const ButtonControlOptions = {
    background: UiColors.base.background,
    active: UiColors.controls.active,
    inactive: UiColors.controls.inactive,
    disabled: UiColors.base.disabled,
    text: UiColors.base.brightText,
    fontSize: 12
};

class ButtonControl
{
    constructor()
    {
        this.presentation = new ButtonPresentation();
        this.pressed = false;
        this.presentationDepth = 0;
        this.presentationDirty = false;
        this.doubleClick = new DoubleClickTracker();
    }

    requestRedraw()
    {
        if (this.presentationDepth > 0)
        {
            this.presentationDirty = true;
            return;
        }
        mgraphics.redraw();
    }

    beginPresentation()
    {
        this.presentationDepth += 1;
    }

    endPresentation()
    {
        if (this.presentationDepth === 0) return;
        this.presentationDepth -= 1;
        if (this.presentationDepth === 0 && this.presentationDirty)
        {
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

    setPresentationValue(value)
    {
    this.presentation.value = Number(value) !== 0;
    this.requestRedraw();
    }

    setPresentationEnabled(value)
    {
    this.presentation.enabled = Number(value) !== 0;
    this.requestRedraw();
    }

    setPresentationActive(value)
    {
    this.presentation.active = Number(value) !== 0;
    this.requestRedraw();
    }

    setPresentationMode(value)
    {
    this.presentation.mode = String(value) === "momentary"
        ? "momentary" : "toggle";
    this.requestRedraw();
    }

    setPresentationLabel(value)
    {
        this.presentation.label = String(value);
        this.requestRedraw();
    }

    setScope(active, hasColor, red, green, blue, alpha)
    {
        this.presentation.scopeActive = Number(active) !== 0;
        this.presentation.scopeColor = Number(hasColor) !== 0
            ? [Number(red), Number(green), Number(blue), Number(alpha)] : null;
        this.requestRedraw();
    }

    emit(name, payload)
    {
    if (payload === undefined) outlet(0, name);
    else if (payload instanceof Array) outlet(0, [name].concat(payload));
    else outlet(0, [name, payload]);
    }

    paint()
    {
    const width = mgraphics.size[0];
    const height = mgraphics.size[1];
    const presentation = this.presentation;
    const selected = Boolean(presentation.value);
    const color = !presentation.enabled
        ? ButtonControlOptions.disabled
        : selected
            ? ButtonControlOptions.active
            : ButtonControlOptions.inactive;

    mgraphics.set_source_rgba.apply(mgraphics, ButtonControlOptions.background);
    mgraphics.rectangle(0, 0, width, height);
    mgraphics.fill();
    mgraphics.set_source_rgba.apply(mgraphics, color);
    mgraphics.rectangle(1, 1, Math.max(0, width - 2), Math.max(0, height - 2));
    if (selected && presentation.enabled) mgraphics.fill();
    else mgraphics.stroke();

        if (presentation.label) {
        mgraphics.select_font_face("Arial");
        mgraphics.set_font_size(ButtonControlOptions.fontSize);
        mgraphics.set_source_rgba.apply(mgraphics, ButtonControlOptions.text);
        const textWidth = String(presentation.label).length
            * ButtonControlOptions.fontSize * 0.55;
        mgraphics.move_to(
            Math.max(2, (width - textWidth) * 0.5),
            height * 0.5 + ButtonControlOptions.fontSize * 0.35
        );
            mgraphics.show_text(String(presentation.label));
        }
        if (presentation.scopeActive && presentation.scopeColor) {
            mgraphics.set_source_rgba.apply(mgraphics, presentation.scopeColor);
            mgraphics.arc(width - 4, 4, 2, 0, Math.PI * 2);
            mgraphics.fill();
        }
    }

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

function applyPresentation(presentation) {
    buttonControl.applyPresentation(presentation);
}

function presentation(presentation) {
    applyPresentation(presentation);
}

function paint() {
    buttonControl.paint();
}

function onresize() {
    mgraphics.redraw();
}

function onclick() {
    buttonControl.click();
}

function scope(active, hasColor, red, green, blue, alpha) {
    buttonControl.setScope(active, hasColor, red, green, blue, alpha);
}

function ondrag(x, y, button) {
    if (button === 0) buttonControl.release();
}

function onidleout() {
    buttonControl.release();
}

function set(value) {
    buttonControl.setPresentationValue(value);
}

function enabled(value) {
    buttonControl.setPresentationEnabled(value);
}

function active(value) {
    buttonControl.setPresentationActive(value);
}

function mode(value) {
    buttonControl.setPresentationMode(value);
}

function label(value) {
    buttonControl.setPresentationLabel(value);
}

function presentation_begin() {
    buttonControl.beginPresentation();
}

function presentation_end() {
    buttonControl.endPresentation();
}

const buttonControl = new ButtonControl();
