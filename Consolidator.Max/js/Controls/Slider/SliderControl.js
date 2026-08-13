autowatch = 1;
inlets = 1;
outlets = 1;

include("../../Presenters/Slider/SliderPresentation.js");

mgraphics.init();
mgraphics.relative_coords = 0;
mgraphics.autofill = 0;

var SliderControlOptions = {
    background: [0.08, 0.08, 0.08, 1],
    track: [0.3, 0.3, 0.3, 1],
    value: [0.35, 0.7, 1, 1],
    disabled: [0.2, 0.2, 0.2, 1],
    padding: 5,
    trackWidth: 3,
    thumbRadius: 5,
    dragSensitivity: 0.005
};

function SliderControl() {
    this.presentation = new SliderPresentation();
    this.previewValue = null;
    this.dragging = false;
}

SliderControl.prototype.applyPresentation = function (presentation) {
    if (!presentation) return;
    this.presentation = presentation;
    if (!this.dragging) this.previewValue = null;
    if (!this.presentation.enabled) {
        this.dragging = false;
        this.previewValue = null;
    }
    mgraphics.redraw();
};

SliderControl.prototype.emit = function (name, payload) {
    if (payload === undefined) outlet(0, name);
    else if (payload instanceof Array) outlet(0, [name].concat(payload));
    else outlet(0, [name, payload]);
};

SliderControl.prototype.value = function () {
    return this.previewValue === null
        ? this.presentation.value : this.previewValue;
};

SliderControl.prototype.setValue = function (value, output) {
    var next = Math.max(
        this.presentation.minimum,
        Math.min(this.presentation.maximum, Number(value))
    );
    if (!isFinite(next)) return;
    this.previewValue = next;
    mgraphics.redraw();
    if (output) this.emit("valueChanged", next);
};

SliderControl.prototype.beginGesture = function (value) {
    if (!this.presentation.enabled || !this.presentation.active) return;
    this.dragging = true;
    this.emit("gestureBegan");
    this.setValue(value, true);
};

SliderControl.prototype.drag = function (value) {
    if (!this.dragging) return;
    this.setValue(value, true);
};

SliderControl.prototype.endGesture = function () {
    if (!this.dragging) return;
    this.dragging = false;
    this.emit("gestureEnded");
};

SliderControl.prototype.paint = function () {
    var width = mgraphics.size[0];
    var height = mgraphics.size[1];
    var vertical = this.presentation.orientation === "vertical";
    var value = this.value();
    var color = this.presentation.enabled
        ? (this.presentation.color || SliderControlOptions.value)
        : SliderControlOptions.disabled;
    var start = SliderControlOptions.padding;
    var end = (vertical ? height : width) - SliderControlOptions.padding;
    var position = start + (end - start) * value;
    var center = vertical ? width * 0.5 : height * 0.5;

    mgraphics.set_source_rgba.apply(mgraphics, SliderControlOptions.background);
    mgraphics.rectangle(0, 0, width, height);
    mgraphics.fill();
    mgraphics.set_source_rgba.apply(mgraphics, SliderControlOptions.track);
    mgraphics.set_line_width(SliderControlOptions.trackWidth);
    mgraphics.new_path();
    if (vertical) mgraphics.move_to(center, start);
    else mgraphics.move_to(start, center);
    if (vertical) mgraphics.line_to(center, end);
    else mgraphics.line_to(end, center);
    mgraphics.stroke();
    mgraphics.set_source_rgba.apply(mgraphics, color);
    mgraphics.set_line_width(SliderControlOptions.trackWidth);
    mgraphics.new_path();
    if (vertical) mgraphics.move_to(center, start);
    else mgraphics.move_to(start, center);
    if (vertical) mgraphics.line_to(center, position);
    else mgraphics.line_to(position, center);
    mgraphics.stroke();
    mgraphics.new_path();
    if (vertical) mgraphics.arc(center, position, SliderControlOptions.thumbRadius, 0, Math.PI * 2);
    else mgraphics.arc(position, center, SliderControlOptions.thumbRadius, 0, Math.PI * 2);
    mgraphics.fill();
};

function applyPresentation(presentation) {
    sliderControl.applyPresentation(presentation);
}

function presentation(presentation) {
    applyPresentation(presentation);
}

function paint() {
    sliderControl.paint();
}

function onclick(x, y) {
    var vertical = sliderControl.presentation.orientation === "vertical";
    var size = vertical ? mgraphics.size[1] : mgraphics.size[0];
    var position = vertical ? y : x;
    var normalized = (position - SliderControlOptions.padding)
        / (size - SliderControlOptions.padding * 2);
    sliderControl.beginGesture(normalized);
}

function ondrag(x, y, button) {
    if (button === 0) sliderControl.endGesture();
    else {
        var position = sliderControl.presentation.orientation === "vertical" ? y : x;
        var size = sliderControl.presentation.orientation === "vertical"
            ? mgraphics.size[1] : mgraphics.size[0];
        var normalized = (position - SliderControlOptions.padding)
            / (size - SliderControlOptions.padding * 2);
        sliderControl.drag(normalized);
    }
}

function onidleout() {
    sliderControl.endGesture();
}

var sliderControl = new SliderControl();
