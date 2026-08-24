autowatch = 1;
inlets = 1;
outlets = 1;

include("Project:/js/Presenters/Button/ButtonPresentation.js");

mgraphics.init();
mgraphics.relative_coords = 0;
mgraphics.autofill = 0;

var ButtonControlOptions = {
    background: [0.08, 0.08, 0.08, 1],
    active: [0.35, 0.7, 1, 1],
    inactive: [0.35, 0.35, 0.35, 1],
    disabled: [0.2, 0.2, 0.2, 1],
    text: [0.9, 0.9, 0.9, 1],
    fontSize: 12
};

function ButtonControl() {
    this.presentation = new ButtonPresentation();
    this.pressed = false;
}

ButtonControl.prototype.applyPresentation = function (presentation) {
    if (!presentation) return;
    this.presentation = presentation;
    if (!presentation.enabled || presentation.mode !== "momentary") {
        this.pressed = false;
    }
    mgraphics.redraw();
};

ButtonControl.prototype.setPresentationValue = function (value) {
    this.presentation.value = Number(value) !== 0;
    mgraphics.redraw();
};

ButtonControl.prototype.setPresentationEnabled = function (value) {
    this.presentation.enabled = Number(value) !== 0;
    mgraphics.redraw();
};

ButtonControl.prototype.setPresentationActive = function (value) {
    this.presentation.active = Number(value) !== 0;
    mgraphics.redraw();
};

ButtonControl.prototype.setPresentationMode = function (value) {
    this.presentation.mode = String(value) === "momentary"
        ? "momentary" : "toggle";
    mgraphics.redraw();
};

ButtonControl.prototype.setPresentationLabel = function (value) {
    this.presentation.label = String(value);
    mgraphics.redraw();
};

ButtonControl.prototype.emit = function (name, payload) {
    if (payload === undefined) outlet(0, name);
    else if (payload instanceof Array) outlet(0, [name].concat(payload));
    else outlet(0, [name, payload]);
};

ButtonControl.prototype.paint = function () {
    var width = mgraphics.size[0];
    var height = mgraphics.size[1];
    var presentation = this.presentation;
    var selected = Boolean(presentation.value);
    var color = !presentation.enabled
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
        var textWidth = String(presentation.label).length
            * ButtonControlOptions.fontSize * 0.55;
        mgraphics.move_to(
            Math.max(2, (width - textWidth) * 0.5),
            height * 0.5 + ButtonControlOptions.fontSize * 0.35
        );
        mgraphics.show_text(String(presentation.label));
    }
};

ButtonControl.prototype.click = function () {
    if (!this.presentation.enabled) return;
    if (this.presentation.mode === "momentary") {
        this.pressed = true;
        this.emit("valueChanged", 1);
        return;
    }
    this.emit("valueChanged", this.presentation.value ? 0 : 1);
};

ButtonControl.prototype.release = function () {
    if (!this.pressed) return;
    this.pressed = false;
    this.emit("valueChanged", 0);
};

function applyPresentation(presentation) {
    buttonControl.applyPresentation(presentation);
}

function presentation(presentation) {
    applyPresentation(presentation);
}

function paint() {
    buttonControl.paint();
}

function onclick() {
    buttonControl.click();
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

var buttonControl = new ButtonControl();
