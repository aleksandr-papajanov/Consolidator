autowatch = 1;
inlets = 1;
outlets = 1;
mgraphics.init();
include("JS/InterfaceVisualConfig.js");
include("JS/ButtonControlModel.js");

var ButtonOptions = {
    mode: "toggle",
    cornerRadiusRatio: 0.12,
    paddingRatio: 0.12
};

declareattribute("mode", "getmode", "setmode", 1);
declareattribute("label", "getlabel", "setlabel", 1);
declareattribute("value", "getvalue", "setvalue", 1);
declareattribute("enabled", "getEnabled", "setEnabled", 1);

function ButtonControl() {
    this.model = new ButtonControlModel(ButtonOptions.mode);
    this.isPressed = false;
    this.enabled = true;
    this.labelText = "Button";
};

ButtonControl.prototype.SetValue = function(value, shouldOutput) {
    var nextValue = this.model.SetValue(value);
    mgraphics.redraw();
    if (shouldOutput) outlet(0, nextValue);
};

ButtonControl.prototype.SetMode = function(mode) {
    if (mode === "toggle" || mode === "momentary") {
        ButtonOptions.mode = mode;
        this.model.SetMode(mode);
        this.labelText = mode === "toggle" ? "Toggle" : "Momentary";
        mgraphics.redraw();
    }
};

ButtonControl.prototype.SetLabel = function(label) {
    this.labelText = String(label);
    mgraphics.redraw();
};

ButtonControl.prototype.Paint = function() {
    var size = mgraphics.size;
    var width = size[0];
    var height = size[1];
    var padding = Math.min(width, height) * ButtonOptions.paddingRatio;
    var radius = Math.min(width, height) * ButtonOptions.cornerRadiusRatio;
    var color = this.enabled && this.model.IsActive()
        ? InterfaceVisualConfig.valueColor
        : InterfaceVisualConfig.trackColor;

    mgraphics.set_source_rgba(color);
    mgraphics.new_path();
    mgraphics.move_to(padding + radius, padding);
    mgraphics.line_to(width - padding - radius, padding);
    mgraphics.curve_to(
        width - padding, padding,
        width - padding, padding,
        width - padding, padding + radius
    );
    mgraphics.line_to(width - padding, height - padding - radius);
    mgraphics.curve_to(
        width - padding, height - padding,
        width - padding, height - padding,
        width - padding - radius, height - padding
    );
    mgraphics.line_to(padding + radius, height - padding);
    mgraphics.curve_to(
        padding, height - padding,
        padding, height - padding,
        padding, height - padding - radius
    );
    mgraphics.line_to(padding, padding + radius);
    mgraphics.curve_to(
        padding, padding,
        padding, padding,
        padding + radius, padding
    );
    mgraphics.close_path();
    mgraphics.fill();

    var fontSize = Math.max(
        InterfaceVisualConfig.minimumTextFontSize,
        Math.min(width, height) * InterfaceVisualConfig.controlFontSizeRatio
    );
    var textSize;
    mgraphics.select_font_face("Arial");
    mgraphics.set_font_size(fontSize);
    textSize = mgraphics.text_measure(this.labelText);
    mgraphics.set_source_rgba(InterfaceVisualConfig.textColor);
    mgraphics.move_to(
        (width - textSize[0]) * 0.5,
        (height + textSize[1]) * 0.5
    );
    mgraphics.show_text(this.labelText);
};

ButtonControl.prototype.HandleClick = function() {
    if (!this.enabled) return;
    this.isPressed = true;
    this.SetValue(this.model.HandleClick(), true);
};

ButtonControl.prototype.HandleRelease = function() {
    if (!this.enabled) return;
    this.isPressed = false;
    var value = this.model.HandleRelease();
    if (value !== null) this.SetValue(value, true);
};

ButtonControl.prototype.HandleMessage = function(value) {
    this.SetValue(value, false);
};

ButtonControl.prototype.SetEnabled = function(value) {
    this.enabled = Number(value) !== 0;
    if (!this.enabled) this.isPressed = false;
    mgraphics.redraw();
};

ButtonControl.prototype.OutputValue = function() {
    outlet(0, this.model.value);
};

var buttonControl = new ButtonControl();

function getmode() {
    return buttonControl.model.mode;
}

function setmode(value) {
    buttonControl.SetMode(String(value));
}

function getlabel() {
    return buttonControl.labelText;
}

function setlabel(value) {
    buttonControl.SetLabel(value);
}

function getvalue() {
    return buttonControl.model.value;
}

function setvalue(value) {
    buttonControl.SetValue(value, false);
}

function getEnabled() {
    return buttonControl.enabled ? 1 : 0;
}

function setEnabled(value) {
    buttonControl.SetEnabled(value);
}

function enabled(value) {
    buttonControl.SetEnabled(value);
}

function enable() {
    buttonControl.SetEnabled(1);
}

function disable() {
    buttonControl.SetEnabled(0);
}

function outputValue() {
    buttonControl.OutputValue();
}

function set(value) {
    buttonControl.SetValue(value, false);
}

function paint() {
    buttonControl.Paint();
}

function msg_float(value) {
    buttonControl.HandleMessage(value);
}

function msg_int(value) {
    buttonControl.HandleMessage(value);
}

function bang() {
    buttonControl.HandleClick();
}

function mode(value) {
    buttonControl.SetMode(String(value));
}

function label() {
    var values = arrayfromargs(arguments);
    buttonControl.SetLabel(values.join(" "));
}

function list() {
    var values = arrayfromargs(arguments);
    if (values.length > 0) buttonControl.HandleMessage(values[0]);
}

function onclick(x, y, button, modifiers, inTime, outTime) {
    buttonControl.HandleClick();
}

function ondrag(x, y, button, modifiers, inTime, outTime) {
    if (button === 0) buttonControl.HandleRelease();
}

function onidleout(x, y, button, modifiers, inTime, outTime) {
    buttonControl.HandleRelease();
}

function onresize(width, height) {
    mgraphics.redraw();
}
