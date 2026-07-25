autowatch = 1;
inlets = 1;
outlets = 1;
mgraphics.init();
include("JS/InterfaceVisualConfig.js");

var SliderOptions = {
    paddingRatio: 0.16,
    dragSensitivity: 0.0
};

declareattribute("value", "getValue", "setValue", 1);
declareattribute("enabled", "getEnabled", "setEnabled", 1);

function SliderControl() {
    this.value = 0.5;
    this.isDragging = false;
    this.enabled = true;
    this.lastX = 0.0;
};

SliderControl.prototype.ClampValue = function(value) {
    return Math.max(0.0, Math.min(1.0, Number(value)));
};

SliderControl.prototype.SetValue = function(value, shouldOutput) {
    var nextValue = this.ClampValue(value);
    if (nextValue === this.value && !shouldOutput) return;
    this.value = nextValue;
    mgraphics.redraw();
    if (shouldOutput) outlet(0, this.value);
};

SliderControl.prototype.GetGeometry = function() {
    var size = mgraphics.size;
    var padding = Math.min(size[0], size[1]) * SliderOptions.paddingRatio;
    return {
        width: size[0],
        height: size[1],
        startX: padding,
        endX: Math.max(padding, size[0] - padding),
        centerY: size[1] * 0.5
    };
};

SliderControl.prototype.Paint = function() {
    var geometry = this.GetGeometry();
    var valueX = geometry.startX
        + (geometry.endX - geometry.startX) * this.value;

    mgraphics.set_line_cap("round");
    mgraphics.set_line_width(InterfaceVisualConfig.controlLineWidth);
    mgraphics.set_source_rgba(InterfaceVisualConfig.trackColor);
    mgraphics.new_path();
    mgraphics.move_to(geometry.startX, geometry.centerY);
    mgraphics.line_to(geometry.endX, geometry.centerY);
    mgraphics.stroke();

    if (this.enabled) mgraphics.set_source_rgba(InterfaceVisualConfig.valueColor);
    else mgraphics.set_source_rgba(InterfaceVisualConfig.trackColor);
    mgraphics.new_path();
    mgraphics.move_to(geometry.startX, geometry.centerY);
    mgraphics.line_to(valueX, geometry.centerY);
    mgraphics.stroke();
};

SliderControl.prototype.HandleDrag = function(x) {
    var geometry = this.GetGeometry();
    var range = Math.max(1.0, geometry.endX - geometry.startX);
    var delta = x - this.lastX;
    this.lastX = x;
    this.SetValue(this.value + delta / range, true);
};

SliderControl.prototype.HandleMessage = function(value) {
    this.SetValue(value, false);
};

SliderControl.prototype.SetEnabled = function(value) {
    this.enabled = Number(value) !== 0;
    if (!this.enabled) this.isDragging = false;
    mgraphics.redraw();
};

SliderControl.prototype.OutputValue = function() {
    outlet(0, this.value);
};

var sliderControl = new SliderControl();

function getValue() {
    return sliderControl.value;
}

function setValue(value) {
    sliderControl.SetValue(value, false);
}

function getEnabled() {
    return sliderControl.enabled ? 1 : 0;
}

function setEnabled(value) {
    sliderControl.SetEnabled(value);
}

function enabled(value) {
    sliderControl.SetEnabled(value);
}

function enable() {
    sliderControl.SetEnabled(1);
}

function disable() {
    sliderControl.SetEnabled(0);
}

function outputValue() {
    sliderControl.OutputValue();
}

function paint() {
    sliderControl.Paint();
}

function msg_float(value) {
    sliderControl.HandleMessage(value);
}

function msg_int(value) {
    sliderControl.HandleMessage(value);
}

function list() {
    var values = arrayfromargs(arguments);
    if (values.length > 0) sliderControl.HandleMessage(values[0]);
}

function onclick(x, y, button, modifiers, inTime, outTime) {
    if (!sliderControl.enabled) return;
    sliderControl.isDragging = true;
    sliderControl.lastX = x;
}

function ondrag(x, y, button, modifiers, inTime, outTime) {
    if (!sliderControl.enabled) return;
    if (sliderControl.isDragging) sliderControl.HandleDrag(x);
}

function onidleout(x, y, button, modifiers, inTime, outTime) {
    sliderControl.isDragging = false;
}

function onresize(width, height) {
    mgraphics.redraw();
}
