autowatch = 1;
inlets = 1;
outlets = 1;

// Inlet: float/int, setValue, limits <minimum> <maximum>, enabled,
// enable, disable, outputValue, displayRange, valueColor, and clearValueColor.
// Outlet: normalized value.
mgraphics.init();
include("JS/InterfaceVisualConfig.js");

var SliderOptions = {
    paddingRatio: 0.16,
    dragSensitivity: 0.0,
    valueAreaRatio: 0.34
};

declareattribute("value", "getValue", "setValue", 1);
declareattribute("enabled", "getEnabled", "setEnabled", 1);

function SliderControl() {
    this.value = 0.5;
    this.minimum = 0.0;
    this.maximum = 1.0;
    this.isDragging = false;
    this.enabled = true;
    this.lastX = 0.0;
    this.displayRange = null;
    this.valueColor = null;
};

SliderControl.prototype.ClampValue = function(value) {
    return Math.max(0.0, Math.min(1.0, Number(value)));
};

SliderControl.prototype.SetValue = function(value, shouldOutput) {
    var nextValue = this.ClampValue(value);
    nextValue = Math.max(this.minimum, Math.min(this.maximum, nextValue));
    if (nextValue === this.value && !shouldOutput) return;
    this.value = nextValue;
    mgraphics.redraw();
    if (shouldOutput) outlet(0, this.value);
};

SliderControl.prototype.SetLimits = function(minimum, maximum) {
    var nextMinimum = this.ClampValue(minimum);
    var nextMaximum = this.ClampValue(maximum);
    if (nextMinimum > nextMaximum) return;
    this.minimum = nextMinimum;
    this.maximum = nextMaximum;
    this.SetValue(this.value, false);
};

SliderControl.prototype.GetGeometry = function() {
    var size = mgraphics.size;
    var padding = Math.min(size[0], size[1]) * SliderOptions.paddingRatio;
    return {
        width: size[0],
        height: size[1],
        startX: Math.max(padding, size[0] * SliderOptions.valueAreaRatio),
        endX: Math.max(padding, size[0] - padding),
        centerY: size[1] * 0.5
    };
};

SliderControl.prototype.SetDisplayRange = function(
    minimum,
    maximum,
    logarithmic,
    decimals,
    suffix
) {
    this.displayRange = {
        minimum: Number(minimum),
        maximum: Number(maximum),
        logarithmic: Number(logarithmic) !== 0,
        decimals: Math.max(0, Math.floor(Number(decimals))),
        suffix: suffix === undefined ? "" : String(suffix)
    };
    mgraphics.redraw();
};

SliderControl.prototype.FormatValue = function() {
    if (!this.displayRange) return this.value.toFixed(2);
    var range = this.displayRange;
    var absolute = range.logarithmic && range.minimum > 0
        ? range.minimum * Math.pow(range.maximum / range.minimum, this.value)
        : range.minimum + this.value * (range.maximum - range.minimum);
    return absolute.toFixed(range.decimals) + range.suffix;
};

SliderControl.prototype.SetValueColor = function(red, green, blue, alpha) {
    this.valueColor = [
        Number(red), Number(green), Number(blue),
        alpha === undefined ? 1 : Number(alpha)
    ];
    mgraphics.redraw();
};

SliderControl.prototype.ClearValueColor = function() {
    this.valueColor = null;
    mgraphics.redraw();
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

    if (this.enabled) {
        mgraphics.set_source_rgba(this.valueColor || InterfaceVisualConfig.valueColor);
    }
    else mgraphics.set_source_rgba(InterfaceVisualConfig.trackColor);
    mgraphics.new_path();
    mgraphics.move_to(geometry.startX, geometry.centerY);
    mgraphics.line_to(valueX, geometry.centerY);
    mgraphics.stroke();

    var text = this.FormatValue();
    var fontSize = Math.max(
        InterfaceVisualConfig.minimumTextFontSize,
        Math.min(geometry.height * 0.48, geometry.startX * 0.28)
    );
    mgraphics.select_font_face("Arial");
    mgraphics.set_font_size(fontSize);
    mgraphics.set_source_rgba(InterfaceVisualConfig.textColor);
    var textSize = mgraphics.text_measure(text);
    mgraphics.move_to(
        Math.max(0, geometry.startX - textSize[0] - SliderOptions.paddingRatio * geometry.height),
        geometry.centerY + textSize[1] * 0.34
    );
    mgraphics.show_text(text);
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

function limits(minimum, maximum) {
    sliderControl.SetLimits(minimum, maximum);
}

function displayRange(minimum, maximum, logarithmic, decimals, suffix) {
    sliderControl.SetDisplayRange(
        minimum, maximum, logarithmic, decimals, suffix);
}

function valueColor(red, green, blue, alpha) {
    sliderControl.SetValueColor(red, green, blue, alpha);
}

function clearValueColor() {
    sliderControl.ClearValueColor();
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
