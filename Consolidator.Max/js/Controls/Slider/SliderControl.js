autowatch = 1;
inlets = 1;
outlets = 1;

mgraphics.init();
mgraphics.relative_coords = 0;
mgraphics.autofill = 0;

const { SliderPresentation } = require("../../Presenters/Slider/SliderPresentation.js");
const { UiColors } = require("../../Theme/UiColors.js");

const SliderControlOptions = {
    background: UiColors.base.background,
    track: UiColors.controls.track,
    value: UiColors.controls.active,
    disabled: UiColors.base.disabled,
    padding: 5,
    trackWidth: 3,
    thumbRadius: 5,
    dragSensitivity: 0.005
};

class SliderControl
{
    constructor()
    {
        this.presentation = new SliderPresentation();
        this.previewValue = null;
        this.dragging = false;
    }

    applyPresentation(presentation)
    {
        if (!presentation) return;
        this.presentation = presentation;
        if (!this.dragging) this.previewValue = null;
        if (!this.presentation.enabled) {
            this.dragging = false;
            this.previewValue = null;
        }
        mgraphics.redraw();
    }

    emit(name, payload)
    {
        if (payload === undefined) outlet(0, name);
        else if (payload instanceof Array) outlet(0, [name].concat(payload));
        else outlet(0, [name, payload]);
    }

    value()
    {
        return this.previewValue === null
            ? this.presentation.value : this.previewValue;
    }

    setValue(value, output)
    {
        let next = Math.max(
            this.presentation.minimum,
            Math.min(this.presentation.maximum, Number(value))
        );
        if (!isFinite(next)) return;
        this.previewValue = next;
        mgraphics.redraw();
        if (output) this.emit("valueChanged", next);
    }

    beginGesture(value)
    {
        if (!this.presentation.enabled || !this.presentation.active) return;
        this.dragging = true;
        this.emit("gestureBegan");
        this.setValue(value, true);
    }

    drag(value)
    {
        if (!this.dragging) return;
        this.setValue(value, true);
    }

    endGesture()
    {
        if (!this.dragging) return;
        this.dragging = false;
        this.emit("gestureEnded");
    }

    paint()
    {
        let width = mgraphics.size[0];
        let height = mgraphics.size[1];
        let vertical = this.presentation.orientation === "vertical";
        let value = this.value();
        let color = this.presentation.enabled
            ? (this.presentation.color || SliderControlOptions.value)
            : SliderControlOptions.disabled;
        let start = SliderControlOptions.padding;
        let end = (vertical ? height : width) - SliderControlOptions.padding;
        let position = start + (end - start) * value;
        let center = vertical ? width * 0.5 : height * 0.5;

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
    }
}

function applyPresentation(presentation) {
    sliderControl.applyPresentation(presentation);
}

function presentation(presentation) {
    applyPresentation(presentation);
}

function paint() {
    sliderControl.paint();
}

function onresize() {
    mgraphics.redraw();
}

function onclick(x, y) {
    let vertical = sliderControl.presentation.orientation === "vertical";
    let size = vertical ? mgraphics.size[1] : mgraphics.size[0];
    let position = vertical ? y : x;
    let normalized = (position - SliderControlOptions.padding)
        / (size - SliderControlOptions.padding * 2);
    sliderControl.beginGesture(normalized);
}

function ondrag(x, y, button) {
    if (button === 0) sliderControl.endGesture();
    else {
        let position = sliderControl.presentation.orientation === "vertical" ? y : x;
        let size = sliderControl.presentation.orientation === "vertical"
            ? mgraphics.size[1] : mgraphics.size[0];
        let normalized = (position - SliderControlOptions.padding)
            / (size - SliderControlOptions.padding * 2);
        sliderControl.drag(normalized);
    }
}

function onidleout() {
    sliderControl.endGesture();
}

const sliderControl = new SliderControl();
