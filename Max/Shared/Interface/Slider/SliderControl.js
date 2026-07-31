autowatch = 1;
inlets = 1;
outlets = 1;

// Inlet: float/int, setValue, limits <minimum> <maximum>, orientation <horizontal|vertical>, enabled,
// enable, disable, outputValue, displayRange, valueColor, and clearValueColor.
// Outlet: normalized value.
mgraphics.init();
include("../../Configuration/InterfaceTheme.js");
include("SliderOptions.js");
include("../Core/ControlState.js");
include("../Core/ControlLayout.js");
include("../Core/ControlRenderer.js");
include("SliderViewModel.js");
include("SliderRenderer.js");

declareattribute("value", "getValue", "setValue", 1);
declareattribute("enabled", "getEnabled", "setEnabled", 1);
declareattribute("orientation", "getOrientation", "setOrientation", 1);

function SliderControl() {
    this.value = 0.5;
    this.minimum = 0.0;
    this.maximum = 1.0;
    this.isDragging = false;
    this.enabled = true;
    this.orientation = "horizontal";
    this.lastX = 0.0;
    this.lastY = 0.0;
    this.displayRange = null;
    this.valueColor = null;
    this.viewModel = new SliderViewModel();
    this.viewState = this.viewModel;
    this.layout = new ControlLayout();
    this.renderer = new SliderRenderer();
};

SliderControl.prototype.ClampValue = function(value) {
    return Math.max(0.0, Math.min(1.0, Number(value)));
};

SliderControl.prototype.SetValue = function(value, shouldOutput) {
    var nextValue = this.ClampValue(value);
    nextValue = Math.max(this.minimum, Math.min(this.maximum, nextValue));
    if (nextValue === this.value && !shouldOutput) return;
    this.value = nextValue;
    this.viewState.SetValue(this.value);
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
    var size = this.layout.Size();
    var padding = Math.min(size.width, size.height) * SliderOptions.paddingRatio;
    if (this.orientation === "vertical") {
        var labelHeight = InterfaceTheme.typography.minimumSize;
        var trackBottom = Math.max(
            padding,
            size.height - padding - labelHeight - SliderOptions.valueGap
        );
        return {
            width: size.width,
            height: size.height,
            startX: size.width * 0.5,
            endX: size.width * 0.5,
            startY: trackBottom,
            endY: padding,
            centerX: size.width * 0.5,
            centerY: size.height * 0.5,
            padding: padding
        };
    }
    return {
        width: size.width,
        height: size.height,
        startX: Math.max(padding, size.width * SliderOptions.valueAreaRatio),
        endX: Math.max(padding, size.width - padding),
        startY: size.height * 0.5,
        endY: size.height * 0.5,
        centerX: size.width * 0.5,
        centerY: size.height * 0.5,
        padding: padding
    };
};

SliderControl.prototype.SetOrientation = function(value) {
    var orientation = String(value).toLowerCase();
    if (orientation !== "horizontal" && orientation !== "vertical") return;
    if (this.orientation === orientation) return;
    this.orientation = orientation;
    this.viewState.SetOrientation(orientation);
    mgraphics.redraw();
};

SliderControl.prototype.GetOrientation = function() {
    return this.orientation;
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
    this.viewState.valueColor = this.valueColor;
    mgraphics.redraw();
};

SliderControl.prototype.ClearValueColor = function() {
    this.valueColor = null;
    this.viewState.valueColor = null;
    mgraphics.redraw();
};

SliderControl.prototype.Paint = function() {
    var geometry = this.GetGeometry();
    this.viewState.enabled = this.enabled;
    this.viewState.value = this.value;
    this.viewState.label = this.FormatValue();
    this.viewState.valueColor = this.valueColor;
    this.renderer.Paint(this.viewState, geometry, InterfaceTheme);
};

SliderControl.prototype.HandleDrag = function(x, y) {
    var geometry = this.GetGeometry();
    var range = this.orientation === "vertical"
        ? Math.max(1.0, geometry.startY - geometry.endY)
        : Math.max(1.0, geometry.endX - geometry.startX);
    var delta = this.orientation === "vertical"
        ? this.lastY - y
        : x - this.lastX;
    this.lastX = x;
    this.lastY = y;
    this.SetValue(this.value + delta / range, true);
};

SliderControl.prototype.HandleMessage = function(value) {
    this.SetValue(value, false);
};

SliderControl.prototype.SetEnabled = function(value) {
    this.enabled = Number(value) !== 0;
    this.viewState.SetEnabled(this.enabled);
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

function getOrientation() {
    return sliderControl.GetOrientation();
}

function setOrientation(value) {
    sliderControl.SetOrientation(value);
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

function orientation(value) {
    sliderControl.SetOrientation(value);
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
    sliderControl.lastY = y;
}

function ondrag(x, y, button, modifiers, inTime, outTime) {
    if (!sliderControl.enabled) return;
    if (sliderControl.isDragging) sliderControl.HandleDrag(x, y);
}

function onidleout(x, y, button, modifiers, inTime, outTime) {
    sliderControl.isDragging = false;
}

function onresize(width, height) {
    mgraphics.redraw();
}
